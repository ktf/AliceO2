// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#include <ROOT/RVec.hxx>
#include "Framework/RootArrowFilesystem.h"
#include "Framework/Endian.h"
#include "Framework/RuntimeError.h"
#include "Framework/Signpost.h"
#include <Rtypes.h>
#include <arrow/array/array_nested.h>
#include <arrow/array/array_primitive.h>
#include <arrow/array/builder_nested.h>
#include <arrow/array/builder_primitive.h>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleWriter.hxx>
#include <memory>
#include <TFile.h>
#include <TLeaf.h>
#include <TBufferFile.h>
#include <TTree.h>
#include <TDirectoryFile.h>
#include <arrow/type.h>
#include <arrow/type_fwd.h>
#include <arrow/dataset/file_base.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/util/key_value_metadata.h>
#include <fmt/format.h>
#include <ROOT/RField.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleReader.hxx>
#include <ROOT/RFieldVisitor.hxx>
#include <ROOT/RNTupleInspector.hxx>
#include <stdexcept>
#include <utility>

template class
  std::unique_ptr<ROOT::Experimental::RNTupleReader>;

template class
  std::shared_ptr<arrow::Array>;

O2_DECLARE_DYNAMIC_LOG(root_arrow_fs);

namespace
{
struct BranchInfo {
  std::string name;
  TBranch* ptr;
  bool mVLA;
};
} // namespace

auto arrowTypeFromROOT(EDataType type, int size)
{
  auto typeGenerator = [](std::shared_ptr<arrow::DataType> const& type, int size) -> std::shared_ptr<arrow::DataType> {
    switch (size) {
      case -1:
        return arrow::list(type);
      case 1:
        return std::move(type);
      default:
        return arrow::fixed_size_list(type, size);
    }
  };

  switch (type) {
    case EDataType::kBool_t:
      return typeGenerator(arrow::boolean(), size);
    case EDataType::kUChar_t:
      return typeGenerator(arrow::uint8(), size);
    case EDataType::kUShort_t:
      return typeGenerator(arrow::uint16(), size);
    case EDataType::kUInt_t:
      return typeGenerator(arrow::uint32(), size);
    case EDataType::kULong64_t:
      return typeGenerator(arrow::uint64(), size);
    case EDataType::kChar_t:
      return typeGenerator(arrow::int8(), size);
    case EDataType::kShort_t:
      return typeGenerator(arrow::int16(), size);
    case EDataType::kInt_t:
      return typeGenerator(arrow::int32(), size);
    case EDataType::kLong64_t:
      return typeGenerator(arrow::int64(), size);
    case EDataType::kFloat_t:
      return typeGenerator(arrow::float32(), size);
    case EDataType::kDouble_t:
      return typeGenerator(arrow::float64(), size);
    default:
      throw o2::framework::runtime_error_f("Unsupported branch type: %d", static_cast<int>(type));
  }
}

struct RootNTupleVisitor : public ROOT::Experimental::Detail::RFieldVisitor {
  void VisitArrayField(const ROOT::Experimental::RArrayField& field) override
  {
    int size = field.GetLength();
    RootNTupleVisitor valueVisitor{};
    auto valueField = field.GetSubFields()[0];
    valueField->AcceptVisitor(valueVisitor);
    auto type = valueVisitor.datatype;
    this->datatype = arrow::fixed_size_list(type, size);
  }

  void VisitRVecField(const ROOT::Experimental::RRVecField& field) override
  {
    RootNTupleVisitor valueVisitor{};
    auto valueField = field.GetSubFields()[0];
    valueField->AcceptVisitor(valueVisitor);
    auto type = valueVisitor.datatype;
    this->datatype = arrow::list(type);
  }

  void VisitField(const ROOT::Experimental::RFieldBase& field) override
  {
    throw o2::framework::runtime_error_f("Unknown field %s with type %s", field.GetFieldName().c_str(), field.GetTypeName().c_str());
  }

  void VisitIntField(const ROOT::Experimental::RField<int>& field) override
  {
    this->datatype = arrow::int32();
  }

  void VisitBoolField(const ROOT::Experimental::RField<bool>& field) override
  {
    this->datatype = arrow::boolean();
  }

  void VisitFloatField(const ROOT::Experimental::RField<float>& field) override
  {
    this->datatype = arrow::float32();
  }

  void VisitDoubleField(const ROOT::Experimental::RField<double>& field) override
  {
    this->datatype = arrow::float64();
  }
  std::shared_ptr<arrow::DataType> datatype;
};

auto arrowTypeFromRNTuple(ROOT::Experimental::RFieldBase const& field, int size)
{
  RootNTupleVisitor visitor;
  field.AcceptVisitor(visitor);
  return visitor.datatype;
}

namespace o2::framework
{
using arrow::Status;

TFileFileSystem::TFileFileSystem(TDirectoryFile* f, size_t readahead)
  : VirtualRootFileSystemBase(),
    mFile(f)
{
  ((TFile*)mFile)->SetReadaheadSize(50 * 1024 * 1024);
}

std::shared_ptr<VirtualRootFileSystemBase> TFileFileSystem::GetSubFilesystem(arrow::dataset::FileSource source)
{
  auto tree = (TTree*)mFile->GetObjectChecked(source.path().c_str(), TClass::GetClass<TTree>());
  if (tree) {
    return std::shared_ptr<VirtualRootFileSystemBase>(new SingleTreeFileSystem(tree));
  }

  auto rntuple = (ROOT::Experimental::RNTuple*)mFile->Get<ROOT::Experimental::RNTuple>(source.path().c_str());
  if (rntuple) {
    return std::shared_ptr<VirtualRootFileSystemBase>(new SingleRNTupleFileSystem(rntuple));
  }

  auto directory = (TDirectoryFile*)mFile->GetObjectChecked(source.path().c_str(), TClass::GetClass<TDirectory>());
  if (directory) {
    return std::shared_ptr<VirtualRootFileSystemBase>(new TFileFileSystem(directory, 50 * 1024 * 1024));
  }
  throw runtime_error_f("Unsupported file layout");
}

arrow::Result<arrow::fs::FileInfo> TFileFileSystem::GetFileInfo(const std::string& path)
{
  arrow::fs::FileInfo result;
  result.set_type(arrow::fs::FileType::NotFound);
  result.set_path(path);
  arrow::dataset::FileSource source(path, shared_from_this());

  auto fs = GetSubFilesystem(source);

  // For now we only support single trees.
  if (std::dynamic_pointer_cast<SingleTreeFileSystem>(fs)) {
    result.set_type(arrow::fs::FileType::File);
    return result;
  }
  return result;
}

arrow::Result<std::shared_ptr<arrow::io::OutputStream>> TFileFileSystem::OpenOutputStream(
  const std::string& path,
  const std::shared_ptr<const arrow::KeyValueMetadata>& metadata)
{
  if (path == "/") {
    return std::make_shared<TDirectoryFileOutputStream>(this->GetFile());
  }

  auto* dir = dynamic_cast<TDirectoryFile*>(this->GetFile()->Get(path.c_str()));
  if (!dir) {
    throw runtime_error_f("Unable to open directory %s in file %s", path.c_str(), GetFile()->GetName());
  }
  auto stream = std::make_shared<TDirectoryFileOutputStream>(dir);
  return stream;
}

arrow::Result<arrow::fs::FileInfo> VirtualRootFileSystemBase::GetFileInfo(std::string const&)
{
  arrow::fs::FileInfo result;
  result.set_type(arrow::fs::FileType::NotFound);
  return result;
}

arrow::Result<arrow::fs::FileInfoVector> VirtualRootFileSystemBase::GetFileInfo(const arrow::fs::FileSelector& select)
{
  arrow::fs::FileInfoVector results;
  auto selected = this->GetFileInfo(select.base_dir);
  if (selected.ok()) {
    results.emplace_back(*selected);
  }
  return results;
}

arrow::Status VirtualRootFileSystemBase::CreateDir(const std::string& path, bool recursive)
{
  return arrow::Status::NotImplemented("Read only filesystem");
}

arrow::Status VirtualRootFileSystemBase::DeleteDir(const std::string& path)
{
  return arrow::Status::NotImplemented("Read only filesystem");
}

arrow::Status VirtualRootFileSystemBase::CopyFile(const std::string& src, const std::string& dest)
{
  return arrow::Status::NotImplemented("Read only filesystem");
}

arrow::Status VirtualRootFileSystemBase::Move(const std::string& src, const std::string& dest)
{
  return arrow::Status::NotImplemented("Read only filesystem");
}

arrow::Status VirtualRootFileSystemBase::DeleteDirContents(const std::string& path, bool missing_dir_ok)
{
  return arrow::Status::NotImplemented("Read only filesystem");
}

arrow::Status VirtualRootFileSystemBase::DeleteRootDirContents()
{
  return arrow::Status::NotImplemented("Read only filesystem");
}

arrow::Status VirtualRootFileSystemBase::DeleteFile(const std::string& path)
{
  return arrow::Status::NotImplemented("Read only filesystem");
}

arrow::Result<std::shared_ptr<arrow::io::InputStream>> VirtualRootFileSystemBase::OpenInputStream(const std::string& path)
{
  return arrow::Status::NotImplemented("Non streamable filesystem");
}

arrow::Result<std::shared_ptr<arrow::io::RandomAccessFile>> VirtualRootFileSystemBase::OpenInputFile(const std::string& path)
{
  return arrow::Status::NotImplemented("No random access file system");
}

arrow::Result<std::shared_ptr<arrow::io::OutputStream>> VirtualRootFileSystemBase::OpenOutputStream(
  const std::string& path,
  const std::shared_ptr<const arrow::KeyValueMetadata>& metadata)
{
  return arrow::Status::NotImplemented("Non streamable filesystem");
}

arrow::Result<std::shared_ptr<arrow::io::OutputStream>> VirtualRootFileSystemBase::OpenAppendStream(
  const std::string& path,
  const std::shared_ptr<const arrow::KeyValueMetadata>& metadata)
{
  return arrow::Status::NotImplemented("No random access file system");
}

arrow::Result<std::shared_ptr<arrow::Schema>> TTreeFileFormat::Inspect(const arrow::dataset::FileSource& source) const
{
  arrow::Schema schema{{}};
  auto fs = std::dynamic_pointer_cast<VirtualRootFileSystemBase>(source.filesystem());
  // Actually get the TTree from the ROOT file.
  auto treeFs = std::dynamic_pointer_cast<TTreeFileSystem>(fs->GetSubFilesystem(source));
  if (!treeFs.get()) {
    throw runtime_error_f("Unknown filesystem %s\n", source.filesystem()->type_name().c_str());
  }
  TTree* tree = treeFs->GetTree(source);

  auto branches = tree->GetListOfBranches();
  auto n = branches->GetEntries();

  std::vector<BranchInfo> branchInfos;
  for (auto i = 0; i < n; ++i) {
    auto branch = static_cast<TBranch*>(branches->At(i));
    auto name = std::string{branch->GetName()};
    auto pos = name.find("_size");
    if (pos != std::string::npos) {
      name.erase(pos);
      branchInfos.emplace_back(BranchInfo{name, (TBranch*)nullptr, true});
    } else {
      auto lookup = std::find_if(branchInfos.begin(), branchInfos.end(), [&](BranchInfo const& bi) {
        return bi.name == name;
      });
      if (lookup == branchInfos.end()) {
        branchInfos.emplace_back(BranchInfo{name, branch, false});
      } else {
        lookup->ptr = branch;
      }
    }
  }

  std::vector<std::shared_ptr<arrow::Field>> fields;
  tree->SetCacheSize(25000000);
  for (auto& bi : branchInfos) {
    static TClass* cls;
    EDataType type;
    bi.ptr->GetExpectedType(cls, type);
    auto listSize = -1;
    if (!bi.mVLA) {
      listSize = static_cast<TLeaf*>(bi.ptr->GetListOfLeaves()->At(0))->GetLenStatic();
    }
    auto field = std::make_shared<arrow::Field>(bi.ptr->GetName(), arrowTypeFromROOT(type, listSize));
    fields.push_back(field);

    tree->AddBranchToCache(bi.ptr);
    if (strncmp(bi.ptr->GetName(), "fIndexArray", strlen("fIndexArray")) == 0) {
      std::string sizeBranchName = bi.ptr->GetName();
      sizeBranchName += "_size";
      auto* sizeBranch = (TBranch*)tree->GetBranch(sizeBranchName.c_str());
      if (sizeBranch) {
        tree->AddBranchToCache(sizeBranch);
      }
    }
  }
  tree->StopCacheLearningPhase();

  return std::make_shared<arrow::Schema>(fields);
}

/// \brief Create a FileFragment for a FileSource.
arrow::Result<std::shared_ptr<arrow::dataset::FileFragment>> TTreeFileFormat::MakeFragment(
  arrow::dataset::FileSource source, arrow::compute::Expression partition_expression,
  std::shared_ptr<arrow::Schema> physical_schema)
{
  std::shared_ptr<arrow::dataset::FileFormat> format = std::make_shared<TTreeFileFormat>(mTotCompressedSize, mTotUncompressedSize);

  auto fragment = std::make_shared<TTreeFileFragment>(std::move(source), std::move(format),
                                                      std::move(partition_expression),
                                                      std::move(physical_schema));
  return std::dynamic_pointer_cast<arrow::dataset::FileFragment>(fragment);
}

arrow::Result<std::shared_ptr<arrow::dataset::FileFragment>> RNTupleFileFormat::MakeFragment(
  arrow::dataset::FileSource source, arrow::compute::Expression partition_expression,
  std::shared_ptr<arrow::Schema> physical_schema)
{
  std::shared_ptr<arrow::dataset::FileFormat> format = std::make_shared<RNTupleFileFormat>(mTotCompressedSize, mTotUncompressedSize);

  auto fragment = std::make_shared<RNTupleFileFragment>(std::move(source), std::move(format),
                                                        std::move(partition_expression),
                                                        std::move(physical_schema));
  return std::dynamic_pointer_cast<arrow::dataset::FileFragment>(fragment);
}

// An arrow outputstream which allows to write to a ttree
TDirectoryFileOutputStream::TDirectoryFileOutputStream(TDirectoryFile* f)
  : mDirectory(f)
{
}

arrow::Status TDirectoryFileOutputStream::Close()
{
  mDirectory->GetFile()->Close();
  return arrow::Status::OK();
}

arrow::Result<int64_t> TDirectoryFileOutputStream::Tell() const
{
  return arrow::Result<int64_t>(arrow::Status::NotImplemented("Cannot move"));
}

arrow::Status TDirectoryFileOutputStream::Write(const void* data, int64_t nbytes)
{
  return arrow::Status::NotImplemented("Cannot write raw bytes to a TTree");
}

bool TDirectoryFileOutputStream::closed() const
{
  return mDirectory->GetFile()->IsOpen() == false;
}

// An arrow outputstream which allows to write to a ttree
// @a branch prefix is to be used to identify a set of branches which all belong to
// the same table.
TTreeOutputStream::TTreeOutputStream(TTree* f, std::string branchPrefix)
  : mTree(f),
    mBranchPrefix(std::move(branchPrefix))
{
}

arrow::Status TTreeOutputStream::Close()
{
  if (mTree->GetCurrentFile() == nullptr) {
    return arrow::Status::Invalid("Cannot close a tree not attached to a file");
  }
  mTree->GetCurrentFile()->Close();
  return arrow::Status::OK();
}

arrow::Result<int64_t> TTreeOutputStream::Tell() const
{
  return arrow::Result<int64_t>(arrow::Status::NotImplemented("Cannot move"));
}

arrow::Status TTreeOutputStream::Write(const void* data, int64_t nbytes)
{
  return arrow::Status::NotImplemented("Cannot write raw bytes to a TTree");
}

bool TTreeOutputStream::closed() const
{
  // A standalone tree is never closed.
  if (mTree->GetCurrentFile() == nullptr) {
    return false;
  }
  return mTree->GetCurrentFile()->IsOpen() == false;
}

TBranch* TTreeOutputStream::CreateBranch(char const* branchName, char const* sizeBranch)
{
  return mTree->Branch((mBranchPrefix + "/" + branchName).c_str(), (char*)nullptr, (mBranchPrefix + sizeBranch).c_str());
}

char const* rootSuffixFromArrow(arrow::Type::type id)
{
  switch (id) {
    case arrow::Type::BOOL:
      return "/O";
    case arrow::Type::UINT8:
      return "/b";
    case arrow::Type::UINT16:
      return "/s";
    case arrow::Type::UINT32:
      return "/i";
    case arrow::Type::UINT64:
      return "/l";
    case arrow::Type::INT8:
      return "/B";
    case arrow::Type::INT16:
      return "/S";
    case arrow::Type::INT32:
      return "/I";
    case arrow::Type::INT64:
      return "/L";
    case arrow::Type::FLOAT:
      return "/F";
    case arrow::Type::DOUBLE:
      return "/D";
    default:
      throw runtime_error("Unsupported arrow column type");
  }
}

std::unique_ptr<ROOT::Experimental::RFieldBase> rootFieldFromArrow(std::shared_ptr<arrow::Field> field, std::string name)
{
  using namespace ROOT::Experimental;
  switch (field->type()->id()) {
    case arrow::Type::BOOL:
      return std::make_unique<RField<bool>>(name);
    case arrow::Type::UINT8:
      return std::make_unique<RField<uint8_t>>(name);
    case arrow::Type::UINT16:
      return std::make_unique<RField<uint16_t>>(name);
    case arrow::Type::UINT32:
      return std::make_unique<RField<uint32_t>>(name);
    case arrow::Type::UINT64:
      return std::make_unique<RField<uint64_t>>(name);
    case arrow::Type::INT8:
      return std::make_unique<RField<int8_t>>(name);
    case arrow::Type::INT16:
      return std::make_unique<RField<int16_t>>(name);
    case arrow::Type::INT32:
      return std::make_unique<RField<int32_t>>(name);
    case arrow::Type::INT64:
      return std::make_unique<RField<int64_t>>(name);
    case arrow::Type::FLOAT:
      return std::make_unique<RField<float>>(name);
    case arrow::Type::DOUBLE:
      return std::make_unique<RField<double>>(name);
    default:
      throw runtime_error("Unsupported arrow column type");
  }
}

class TTreeFileWriter : public arrow::dataset::FileWriter
{
  std::vector<TBranch*> branches;
  std::vector<TBranch*> sizesBranches;
  std::vector<std::shared_ptr<arrow::Array>> valueArrays;
  std::vector<std::shared_ptr<arrow::Array>> sizeArrays;
  std::vector<std::shared_ptr<arrow::DataType>> valueTypes;

  std::vector<int64_t> valuesIdealBasketSize;
  std::vector<int64_t> sizeIdealBasketSize;

  std::vector<int64_t> typeSizes;
  std::vector<int64_t> listSizes;
  bool firstBasket = true;

  // This is to create a batsket size according to the first batch.
  void finaliseBasketSize(std::shared_ptr<arrow::RecordBatch> firstBatch)
  {
    O2_SIGNPOST_ID_FROM_POINTER(sid, root_arrow_fs, this);
    O2_SIGNPOST_START(root_arrow_fs, sid, "finaliseBasketSize", "First batch with %lli rows received and %zu columns",
                      firstBatch->num_rows(), firstBatch->columns().size());
    for (size_t i = 0; i < branches.size(); i++) {
      auto* branch = branches[i];
      auto* sizeBranch = sizesBranches[i];

      int valueSize = valueTypes[i]->byte_width();
      if (listSizes[i] == 1) {
        O2_SIGNPOST_EVENT_EMIT(root_arrow_fs, sid, "finaliseBasketSize", "Branch %s exists and uses %d bytes per entry for %lli entries.",
                               branch->GetName(), valueSize, firstBatch->num_rows());
        assert(sizeBranch == nullptr);
        branch->SetBasketSize(1024 + firstBatch->num_rows() * valueSize);
      } else if (listSizes[i] == -1) {
        O2_SIGNPOST_EVENT_EMIT(root_arrow_fs, sid, "finaliseBasketSize", "Branch %s exists and uses %d bytes per entry.",
                               branch->GetName(), valueSize);
        // This should probably lookup the
        auto column = firstBatch->GetColumnByName(schema_->field(i)->name());
        auto list = std::static_pointer_cast<arrow::ListArray>(column);
        O2_SIGNPOST_EVENT_EMIT(root_arrow_fs, sid, "finaliseBasketSize", "Branch %s needed. Associated size branch %s and there are %lli entries of size %d in that list.",
                               branch->GetName(), sizeBranch->GetName(), list->length(), valueSize);
        branch->SetBasketSize(1024 + firstBatch->num_rows() * valueSize * list->length());
        sizeBranch->SetBasketSize(1024 + firstBatch->num_rows() * 4);
      } else {
        O2_SIGNPOST_EVENT_EMIT(root_arrow_fs, sid, "finaliseBasketSize", "Branch %s needed. There are %lli entries per array of size %d in that list.",
                               branch->GetName(), listSizes[i], valueSize);
        assert(sizeBranch == nullptr);
        branch->SetBasketSize(1024 + firstBatch->num_rows() * valueSize * listSizes[i]);
      }

      auto field = firstBatch->schema()->field(i);
      if (field->name().starts_with("fIndexArray")) {
        // One int per array to keep track of the size
        int idealBasketSize = 4 * firstBatch->num_rows() + 1024 + field->type()->byte_width() * firstBatch->num_rows(); // minimal additional size needed, otherwise we get 2 baskets
        int basketSize = std::max(32000, idealBasketSize);                                                              // keep a minimum value
        sizeBranch->SetBasketSize(basketSize);
        branch->SetBasketSize(basketSize);
      }
    }
    O2_SIGNPOST_END(root_arrow_fs, sid, "finaliseBasketSize", "Done");
  }

 public:
  // Create the TTree based on the physical_schema, not the one in the batch.
  // The write method will have to reconcile the two schemas.
  TTreeFileWriter(std::shared_ptr<arrow::Schema> schema, std::shared_ptr<arrow::dataset::FileWriteOptions> options,
                  std::shared_ptr<arrow::io::OutputStream> destination,
                  arrow::fs::FileLocator destination_locator)
    : FileWriter(schema, options, destination, destination_locator)
  {
    // Batches have the same number of entries for each column.
    auto directoryStream = std::dynamic_pointer_cast<TDirectoryFileOutputStream>(destination_);
    auto treeStream = std::dynamic_pointer_cast<TTreeOutputStream>(destination_);

    if (directoryStream.get()) {
      TDirectoryFile* dir = directoryStream->GetDirectory();
      dir->cd();
      auto* tree = new TTree(destination_locator_.path.c_str(), "");
      treeStream = std::make_shared<TTreeOutputStream>(tree, "");
    } else if (treeStream.get()) {
      // We already have a tree stream, let's derive a new one
      // with the destination_locator_.path as prefix for the branches
      // This way we can multiplex multiple tables in the same tree.
      auto tree = treeStream->GetTree();
      treeStream = std::make_shared<TTreeOutputStream>(tree, destination_locator_.path);
    } else {
      // I could simply set a prefix here to merge to an already existing tree.
      throw std::runtime_error("Unsupported backend.");
    }

    for (auto i = 0u; i < schema->fields().size(); ++i) {
      auto& field = schema->field(i);
      listSizes.push_back(1);

      int valuesIdealBasketSize = 0;
      // Construct all the needed branches.
      switch (field->type()->id()) {
        case arrow::Type::FIXED_SIZE_LIST: {
          listSizes.back() = std::static_pointer_cast<arrow::FixedSizeListType>(field->type())->list_size();
          valuesIdealBasketSize = 1024 + valueTypes.back()->byte_width() * listSizes.back();
          valueTypes.push_back(field->type()->field(0)->type());
          sizesBranches.push_back(nullptr);
          std::string leafList = fmt::format("{}[{}]{}", field->name(), listSizes.back(), rootSuffixFromArrow(valueTypes.back()->id()));
          branches.push_back(treeStream->CreateBranch(field->name().c_str(), leafList.c_str()));
        } break;
        case arrow::Type::LIST: {
          valueTypes.push_back(field->type()->field(0)->type());
          std::string leafList = fmt::format("{}[{}_size]{}", field->name(), field->name(), rootSuffixFromArrow(valueTypes.back()->id()));
          listSizes.back() = -1; // VLA, we need to calculate it on the fly;
          std::string sizeLeafList = field->name() + "_size/I";
          sizesBranches.push_back(treeStream->CreateBranch((field->name() + "_size").c_str(), sizeLeafList.c_str()));
          branches.push_back(treeStream->CreateBranch(field->name().c_str(), leafList.c_str()));
          // Notice that this could be replaced by a better guess of the
          // average size of the list elements, but this is not trivial.
        } break;
        default: {
          valueTypes.push_back(field->type());
          std::string leafList = field->name() + rootSuffixFromArrow(valueTypes.back()->id());
          sizesBranches.push_back(nullptr);
          branches.push_back(treeStream->CreateBranch(field->name().c_str(), leafList.c_str()));
        } break;
      }
    }
    // We create the branches from the schema
  }

  arrow::Status Write(const std::shared_ptr<arrow::RecordBatch>& batch) override
  {
    if (firstBasket) {
      firstBasket = false;
      finaliseBasketSize(batch);
    }

    // Support writing empty tables
    if (batch->columns().empty() || batch->num_rows() == 0) {
      return arrow::Status::OK();
    }

    // Batches have the same number of entries for each column.
    auto directoryStream = std::dynamic_pointer_cast<TDirectoryFileOutputStream>(destination_);
    TTree* tree = nullptr;
    if (directoryStream.get()) {
      TDirectoryFile* dir = directoryStream->GetDirectory();
      tree = (TTree*)dir->Get(destination_locator_.path.c_str());
    }
    auto treeStream = std::dynamic_pointer_cast<TTreeOutputStream>(destination_);

    if (!tree) {
      // I could simply set a prefix here to merge to an already existing tree.
      throw std::runtime_error("Unsupported backend.");
    }

    for (auto i = 0u; i < batch->columns().size(); ++i) {
      auto column = batch->column(i);
      auto& field = batch->schema()->field(i);

      valueArrays.push_back(nullptr);

      switch (field->type()->id()) {
        case arrow::Type::FIXED_SIZE_LIST: {
          auto list = std::static_pointer_cast<arrow::FixedSizeListArray>(column);
          valueArrays.back() = list->values();
        } break;
        case arrow::Type::LIST: {
          auto list = std::static_pointer_cast<arrow::ListArray>(column);
          valueArrays.back() = list;
        } break;
        case arrow::Type::BOOL: {
          // In case of arrays of booleans, we need to go back to their
          // char based representation for ROOT to save them.
          auto boolArray = std::static_pointer_cast<arrow::BooleanArray>(column);

          int64_t length = boolArray->length();
          arrow::UInt8Builder builder;
          auto ok = builder.Reserve(length);

          for (int64_t i = 0; i < length; ++i) {
            if (boolArray->IsValid(i)) {
              // Expand each boolean value (true/false) to uint8 (1/0)
              uint8_t value = boolArray->Value(i) ? 1 : 0;
              auto ok = builder.Append(value);
            } else {
              // Append null for invalid entries
              auto ok = builder.AppendNull();
            }
          }
          valueArrays.back() = *builder.Finish();
        } break;
        default:
          valueArrays.back() = column;
      }
    }

    int64_t pos = 0;
    while (pos < batch->num_rows()) {
      for (size_t bi = 0; bi < branches.size(); ++bi) {
        auto* branch = branches[bi];
        auto* sizeBranch = sizesBranches[bi];
        auto array = batch->column(bi);
        auto& field = batch->schema()->field(bi);
        auto& listSize = listSizes[bi];
        auto valueType = valueTypes[bi];
        auto valueArray = valueArrays[bi];

        switch (field->type()->id()) {
          case arrow::Type::LIST: {
            auto list = std::static_pointer_cast<arrow::ListArray>(array);
            listSize = list->value_length(pos);
            uint8_t const* buffer = std::static_pointer_cast<arrow::PrimitiveArray>(valueArray)->values()->data() + array->offset() + list->value_offset(pos) * valueType->byte_width();
            branch->SetAddress((void*)buffer);
            sizeBranch->SetAddress(&listSize);
          };
            break;
          case arrow::Type::FIXED_SIZE_LIST:
          default: {
            uint8_t const* buffer = std::static_pointer_cast<arrow::PrimitiveArray>(valueArray)->values()->data() + array->offset() + pos * listSize * valueType->byte_width();
            branch->SetAddress((void*)buffer);
          };
        }
      }
      tree->Fill();
      ++pos;
    }
    return arrow::Status::OK();
  }

  arrow::Future<> FinishInternal() override
  {
    auto treeStream = std::dynamic_pointer_cast<TTreeOutputStream>(destination_);
    TTree* tree = treeStream->GetTree();
    tree->Write("", TObject::kOverwrite);
    tree->SetDirectory(nullptr);

    return {};
  };
};

class RNTupleFileWriter : public arrow::dataset::FileWriter
{
  std::shared_ptr<ROOT::Experimental::RNTupleWriter> mWriter;
  bool firstBatch = true;
  std::vector<std::shared_ptr<arrow::Array>> valueArrays;
  std::vector<std::shared_ptr<arrow::DataType>> valueTypes;
  std::vector<size_t> valueCount;

 public:
  RNTupleFileWriter(std::shared_ptr<arrow::Schema> schema, std::shared_ptr<arrow::dataset::FileWriteOptions> options,
                    std::shared_ptr<arrow::io::OutputStream> destination,
                    arrow::fs::FileLocator destination_locator)
    : FileWriter(schema, options, destination, destination_locator)
  {
    using namespace ROOT::Experimental;

    auto model = RNTupleModel::Create();
    // Let's create a model from the physical schema
    for (auto i = 0u; i < schema->fields().size(); ++i) {
      auto& field = schema->field(i);

      // Construct all the needed branches.
      switch (field->type()->id()) {
        case arrow::Type::FIXED_SIZE_LIST: {
          auto list = std::static_pointer_cast<arrow::FixedSizeListType>(field->type());
          auto valueField = field->type()->field(0);
          model->AddField(std::make_unique<RArrayField>(field->name(), rootFieldFromArrow(valueField, "_0"), list->list_size()));
        } break;
        case arrow::Type::LIST: {
          auto valueField = field->type()->field(0);
          model->AddField(std::make_unique<RRVecField>(field->name(), rootFieldFromArrow(valueField, "_0")));
        } break;
        default: {
          model->AddField(rootFieldFromArrow(field, field->name()));
        } break;
      }
    }
    auto fileStream = std::dynamic_pointer_cast<TDirectoryFileOutputStream>(destination_);
    auto* file = dynamic_cast<TFile*>(fileStream->GetDirectory());
    mWriter = RNTupleWriter::Append(std::move(model), destination_locator_.path, *file, {});
  }

  arrow::Status Write(const std::shared_ptr<arrow::RecordBatch>& batch) override
  {
    if (firstBatch) {
      firstBatch = false;
    }

    // Support writing empty tables
    if (batch->columns().empty() || batch->num_rows() == 0) {
      return arrow::Status::OK();
    }

    for (auto i = 0u; i < batch->columns().size(); ++i) {
      auto column = batch->column(i);
      auto& field = batch->schema()->field(i);

      valueArrays.push_back(nullptr);
      valueTypes.push_back(nullptr);
      valueCount.push_back(1);

      switch (field->type()->id()) {
        case arrow::Type::FIXED_SIZE_LIST: {
          auto list = std::static_pointer_cast<arrow::FixedSizeListArray>(column);
          auto listType = std::static_pointer_cast<arrow::FixedSizeListType>(field->type());
          if (field->type()->field(0)->type()->id() == arrow::Type::BOOL) {
            auto boolArray = std::static_pointer_cast<arrow::BooleanArray>(list->values());
            int64_t length = boolArray->length();
            arrow::UInt8Builder builder;
            auto ok = builder.Reserve(length);

            for (int64_t i = 0; i < length; ++i) {
              if (boolArray->IsValid(i)) {
                // Expand each boolean value (true/false) to uint8 (1/0)
                uint8_t value = boolArray->Value(i) ? 1 : 0;
                auto ok = builder.Append(value);
              } else {
                // Append null for invalid entries
                auto ok = builder.AppendNull();
              }
            }
            valueArrays.back() = *builder.Finish();
            valueTypes.back() = valueArrays.back()->type();
          } else {
            valueArrays.back() = list->values();
            valueTypes.back() = field->type()->field(0)->type();
          }
          valueCount.back() = listType->list_size();
        } break;
        case arrow::Type::LIST: {
          auto list = std::static_pointer_cast<arrow::ListArray>(column);
          valueArrays.back() = list;
          valueTypes.back() = field->type()->field(0)->type();
          valueCount.back() = -1;
        } break;
        case arrow::Type::BOOL: {
          // We unpack the array
          auto boolArray = std::static_pointer_cast<arrow::BooleanArray>(column);
          int64_t length = boolArray->length();
          arrow::UInt8Builder builder;
          auto ok = builder.Reserve(length);

          for (int64_t i = 0; i < length; ++i) {
            if (boolArray->IsValid(i)) {
              // Expand each boolean value (true/false) to uint8 (1/0)
              uint8_t value = boolArray->Value(i) ? 1 : 0;
              auto ok = builder.Append(value);
            } else {
              // Append null for invalid entries
              auto ok = builder.AppendNull();
            }
          }
          valueArrays.back() = *builder.Finish();
          valueTypes.back() = valueArrays.back()->type();
        } break;
        default:
          valueArrays.back() = column;
          valueTypes.back() = field->type();
          break;
      }
    }

    int64_t pos = 0;

    auto entry = mWriter->CreateEntry();
    while (pos < batch->num_rows()) {
      for (size_t ci = 0; ci < batch->columns().size(); ++ci) {
        auto type = batch->column(ci)->type();
        auto field = batch->schema()->field(ci);
        auto token = entry->GetToken(field->name());

        switch (type->id()) {
          case arrow::Type::LIST: {
            auto list = std::static_pointer_cast<arrow::ListArray>(valueArrays[ci]);
            auto value_slice = list->value_slice(pos);

            valueCount[ci] = value_slice->length();
            auto bindValue = [&vc = valueCount, ci, token](auto array, std::unique_ptr<ROOT::Experimental::REntry>& entry) -> void {
              using value_type = std::decay_t<decltype(*array.get())>::value_type;
              auto v = std::make_shared<ROOT::VecOps::RVec<value_type>>((value_type*)array->raw_values(), vc[ci]);
              entry->BindValue(token, v);
            };
            switch (valueTypes[ci]->id()) {
              case arrow::Type::FLOAT: {
                bindValue(std::static_pointer_cast<arrow::FloatArray>(value_slice), entry);
              } break;
              case arrow::Type::DOUBLE: {
                bindValue(std::static_pointer_cast<arrow::DoubleArray>(value_slice), entry);
              } break;
              case arrow::Type::INT8: {
                bindValue(std::static_pointer_cast<arrow::Int8Array>(value_slice), entry);
              } break;
              case arrow::Type::INT16: {
                bindValue(std::static_pointer_cast<arrow::Int16Array>(value_slice), entry);
              } break;
              case arrow::Type::INT32: {
                bindValue(std::static_pointer_cast<arrow::Int32Array>(value_slice), entry);
              } break;
              case arrow::Type::INT64: {
                bindValue(std::static_pointer_cast<arrow::Int64Array>(value_slice), entry);
              } break;
              case arrow::Type::UINT8: {
                bindValue(std::static_pointer_cast<arrow::UInt8Array>(value_slice), entry);
              } break;
              case arrow::Type::UINT16: {
                bindValue(std::static_pointer_cast<arrow::UInt16Array>(value_slice), entry);
              } break;
              case arrow::Type::UINT32: {
                bindValue(std::static_pointer_cast<arrow::UInt32Array>(value_slice), entry);
              } break;
              case arrow::Type::UINT64: {
                bindValue(std::static_pointer_cast<arrow::UInt64Array>(value_slice), entry);
              } break;
              default: {
                throw runtime_error("Unsupported kind of VLA");
              } break;
            }
          } break;
          case arrow::Type::FIXED_SIZE_LIST: {
            entry->BindRawPtr<void>(token, (void*)(valueArrays[ci]->data()->buffers[1]->data() + pos * valueCount[ci] * valueTypes[ci]->byte_width()));
          } break;
          case arrow::Type::BOOL: {
            // Not sure we actually need this
            entry->BindRawPtr<bool>(token, (bool*)(valueArrays[ci]->data()->buffers[1]->data() + pos * 1));
          } break;
          default:
            // By default we consider things scalars.
            entry->BindRawPtr<void>(token, (void*)(valueArrays[ci]->data()->buffers[1]->data() + pos * valueTypes[ci]->byte_width()));
            break;
        }
      }
      mWriter->Fill(*entry);
      ++pos;
    }
    mWriter->CommitCluster();

    return arrow::Status::OK();
  }

  arrow::Future<>
    FinishInternal() override
  {
    return {};
  };
};

arrow::Result<std::shared_ptr<arrow::dataset::FileWriter>> TTreeFileFormat::MakeWriter(std::shared_ptr<arrow::io::OutputStream> destination, std::shared_ptr<arrow::Schema> schema, std::shared_ptr<arrow::dataset::FileWriteOptions> options, arrow::fs::FileLocator destination_locator) const
{
  auto writer = std::make_shared<TTreeFileWriter>(schema, options, destination, destination_locator);
  return std::dynamic_pointer_cast<arrow::dataset::FileWriter>(writer);
}

std::shared_ptr<arrow::dataset::FileWriteOptions> TTreeFileFormat::DefaultWriteOptions()
{
  std::shared_ptr<TTreeFileWriteOptions> options(
    new TTreeFileWriteOptions(shared_from_this()));
  return options;
}

arrow::Result<arrow::RecordBatchGenerator> TTreeFileFormat::ScanBatchesAsync(
  const std::shared_ptr<arrow::dataset::ScanOptions>& options,
  const std::shared_ptr<arrow::dataset::FileFragment>& fragment) const
{
  // Get the fragment as a TTreeFragment. This might be PART of a TTree.
  auto treeFragment = std::dynamic_pointer_cast<TTreeFileFragment>(fragment);
  // This is the schema we want to read
  auto dataset_schema = options->dataset_schema;

  auto generator = [pool = options->pool, treeFragment, dataset_schema, &totalCompressedSize = mTotCompressedSize,
                    &totalUncompressedSize = mTotUncompressedSize]() -> arrow::Future<std::shared_ptr<arrow::RecordBatch>> {
    auto schema = treeFragment->format()->Inspect(treeFragment->source());

    std::vector<std::shared_ptr<arrow::Array>> columns;
    std::vector<std::shared_ptr<arrow::Field>> fields = dataset_schema->fields();
    auto physical_schema = *treeFragment->ReadPhysicalSchema();

    static TBufferFile buffer{TBuffer::EMode::kWrite, 4 * 1024 * 1024};
    auto containerFS = std::dynamic_pointer_cast<VirtualRootFileSystemBase>(treeFragment->source().filesystem());
    auto fs = std::dynamic_pointer_cast<TTreeFileSystem>(containerFS->GetSubFilesystem(treeFragment->source()));

    int64_t rows = -1;
    TTree* tree = fs->GetTree(treeFragment->source());
    for (auto& field : fields) {
      // The field actually on disk
      auto physicalField = physical_schema->GetFieldByName(field->name());
      TBranch* branch = tree->GetBranch(physicalField->name().c_str());
      assert(branch);
      buffer.Reset();
      auto totalEntries = branch->GetEntries();
      if (rows == -1) {
        rows = totalEntries;
      }
      if (rows != totalEntries) {
        throw runtime_error_f("Unmatching number of rows for branch %s", branch->GetName());
      }
      arrow::Status status;
      int readEntries = 0;
      std::shared_ptr<arrow::Array> array;
      auto listType = std::dynamic_pointer_cast<arrow::FixedSizeListType>(physicalField->type());
      if (physicalField->type() == arrow::boolean() ||
          (listType && physicalField->type()->field(0)->type() == arrow::boolean())) {
        if (listType) {
          std::unique_ptr<arrow::ArrayBuilder> builder = nullptr;
          auto status = arrow::MakeBuilder(pool, physicalField->type()->field(0)->type(), &builder);
          if (!status.ok()) {
            throw runtime_error("Cannot create value builder");
          }
          auto listBuilder = std::make_unique<arrow::FixedSizeListBuilder>(pool, std::move(builder), listType->list_size());
          auto valueBuilder = listBuilder.get()->value_builder();
          // boolean array special case: we need to use builder to create the bitmap
          status = valueBuilder->Reserve(totalEntries * listType->list_size());
          status &= listBuilder->Reserve(totalEntries);
          if (!status.ok()) {
            throw runtime_error("Failed to reserve memory for array builder");
          }
          while (readEntries < totalEntries) {
            auto readLast = branch->GetBulkRead().GetBulkEntries(readEntries, buffer);
            readEntries += readLast;
            status &= static_cast<arrow::BooleanBuilder*>(valueBuilder)->AppendValues(reinterpret_cast<uint8_t const*>(buffer.GetCurrent()), readLast * listType->list_size());
          }
          status &= static_cast<arrow::FixedSizeListBuilder*>(listBuilder.get())->AppendValues(readEntries);
          if (!status.ok()) {
            throw runtime_error("Failed to append values to array");
          }
          status &= listBuilder->Finish(&array);
          if (!status.ok()) {
            throw runtime_error("Failed to create array");
          }
        } else if (listType == nullptr) {
          std::unique_ptr<arrow::ArrayBuilder> builder = nullptr;
          auto status = arrow::MakeBuilder(pool, physicalField->type(), &builder);
          if (!status.ok()) {
            throw runtime_error("Cannot create builder");
          }
          auto valueBuilder = static_cast<arrow::BooleanBuilder*>(builder.get());
          // boolean array special case: we need to use builder to create the bitmap
          status = valueBuilder->Reserve(totalEntries);
          if (!status.ok()) {
            throw runtime_error("Failed to reserve memory for array builder");
          }
          while (readEntries < totalEntries) {
            auto readLast = branch->GetBulkRead().GetBulkEntries(readEntries, buffer);
            readEntries += readLast;
            status &= valueBuilder->AppendValues(reinterpret_cast<uint8_t const*>(buffer.GetCurrent()), readLast);
          }
          if (!status.ok()) {
            throw runtime_error("Failed to append values to array");
          }
          status &= valueBuilder->Finish(&array);
          if (!status.ok()) {
            throw runtime_error("Failed to create array");
          }
        }
      } else {
        // other types: use serialized read to build arrays directly.
        auto typeSize = physicalField->type()->byte_width();
        // This is needed for branches which have not been persisted.
        auto bytes = branch->GetTotBytes();
        auto branchSize = bytes ? bytes : 1000000;
        auto&& result = arrow::AllocateResizableBuffer(branchSize, pool);
        if (!result.ok()) {
          throw runtime_error("Cannot allocate values buffer");
        }
        std::shared_ptr<arrow::Buffer> arrowValuesBuffer = std::move(result).ValueUnsafe();
        auto ptr = arrowValuesBuffer->mutable_data();
        if (ptr == nullptr) {
          throw runtime_error("Invalid buffer");
        }

        std::unique_ptr<TBufferFile> offsetBuffer = nullptr;

        uint32_t offset = 0;
        int count = 0;
        std::shared_ptr<arrow::Buffer> arrowOffsetBuffer;
        std::span<int> offsets;
        int size = 0;
        uint32_t totalSize = 0;
        TBranch* mSizeBranch = nullptr;
        int64_t listSize = 1;
        if (auto fixedSizeList = std::dynamic_pointer_cast<arrow::FixedSizeListType>(physicalField->type())) {
          listSize = fixedSizeList->list_size();
          typeSize = fixedSizeList->field(0)->type()->byte_width();
        } else if (auto vlaListType = std::dynamic_pointer_cast<arrow::ListType>(physicalField->type())) {
          listSize = -1;
          typeSize = vlaListType->field(0)->type()->byte_width();
        }
        if (listSize == -1) {
          mSizeBranch = branch->GetTree()->GetBranch((std::string{branch->GetName()} + "_size").c_str());
          offsetBuffer = std::make_unique<TBufferFile>(TBuffer::EMode::kWrite, 4 * 1024 * 1024);
          result = arrow::AllocateResizableBuffer((totalEntries + 1) * (int64_t)sizeof(int), pool);
          if (!result.ok()) {
            throw runtime_error("Cannot allocate offset buffer");
          }
          arrowOffsetBuffer = std::move(result).ValueUnsafe();
          unsigned char* ptrOffset = arrowOffsetBuffer->mutable_data();
          auto* tPtrOffset = reinterpret_cast<int*>(ptrOffset);
          offsets = std::span<int>{tPtrOffset, tPtrOffset + totalEntries + 1};

          // read sizes first
          while (readEntries < totalEntries) {
            auto readLast = mSizeBranch->GetBulkRead().GetEntriesSerialized(readEntries, *offsetBuffer);
            readEntries += readLast;
            for (auto i = 0; i < readLast; ++i) {
              offsets[count++] = (int)offset;
              offset += swap32_(reinterpret_cast<uint32_t*>(offsetBuffer->GetCurrent())[i]);
            }
          }
          offsets[count] = (int)offset;
          totalSize = offset;
          readEntries = 0;
        }

        while (readEntries < totalEntries) {
          auto readLast = branch->GetBulkRead().GetEntriesSerialized(readEntries, buffer);
          if (listSize == -1) {
            size = offsets[readEntries + readLast] - offsets[readEntries];
          } else {
            size = readLast * listSize;
          }
          readEntries += readLast;
          swapCopy(ptr, buffer.GetCurrent(), size, typeSize);
          ptr += (ptrdiff_t)(size * typeSize);
        }
        if (listSize >= 1) {
          totalSize = readEntries * listSize;
        }
        std::shared_ptr<arrow::PrimitiveArray> varray;
        switch (listSize) {
          case -1:
            varray = std::make_shared<arrow::PrimitiveArray>(physicalField->type()->field(0)->type(), totalSize, arrowValuesBuffer);
            array = std::make_shared<arrow::ListArray>(physicalField->type(), readEntries, arrowOffsetBuffer, varray);
            break;
          case 1:
            array = std::make_shared<arrow::PrimitiveArray>(physicalField->type(), readEntries, arrowValuesBuffer);
            break;
          default:
            varray = std::make_shared<arrow::PrimitiveArray>(physicalField->type()->field(0)->type(), totalSize, arrowValuesBuffer);
            array = std::make_shared<arrow::FixedSizeListArray>(physicalField->type(), readEntries, varray);
        }
      }

      branch->SetStatus(false);
      branch->DropBaskets("all");
      branch->Reset();
      branch->GetTransientBuffer(0)->Expand(0);

      columns.push_back(array);
    }
    auto batch = arrow::RecordBatch::Make(dataset_schema, rows, columns);
    totalCompressedSize += tree->GetZipBytes();
    totalUncompressedSize += tree->GetTotBytes();
    return batch;
  };
  return generator;
}

arrow::Result<std::shared_ptr<arrow::Schema>> RNTupleFileFormat::Inspect(const arrow::dataset::FileSource& source) const
{

  auto fs = std::dynamic_pointer_cast<VirtualRootFileSystemBase>(source.filesystem());
  // Actually get the TTree from the ROOT file.
  auto ntupleFs = std::dynamic_pointer_cast<RNTupleFileSystem>(fs->GetSubFilesystem(source));
  if (!ntupleFs.get()) {
    throw runtime_error_f("Unknown filesystem %s\n", source.filesystem()->type_name().c_str());
  }
  ROOT::Experimental::RNTuple* rntuple = ntupleFs->GetRNTuple(source);

  auto inspector = ROOT::Experimental::RNTupleInspector::Create(rntuple);

  auto reader = ROOT::Experimental::RNTupleReader::Open(rntuple);

  auto& tupleField0 = reader->GetModel().GetFieldZero();
  std::vector<std::shared_ptr<arrow::Field>> fields;
  for (auto& tupleField : tupleField0.GetSubFields()) {
    auto field = std::make_shared<arrow::Field>(tupleField->GetFieldName(), arrowTypeFromRNTuple(*tupleField, tupleField->GetValueSize()));
    fields.push_back(field);
  }

  return std::make_shared<arrow::Schema>(fields);
}

arrow::Result<arrow::RecordBatchGenerator> RNTupleFileFormat::ScanBatchesAsync(
  const std::shared_ptr<arrow::dataset::ScanOptions>& options,
  const std::shared_ptr<arrow::dataset::FileFragment>& fragment) const
{
  auto dataset_schema = options->dataset_schema;
  auto ntupleFragment = std::dynamic_pointer_cast<RNTupleFileFragment>(fragment);

  auto generator = [pool = options->pool, ntupleFragment, dataset_schema, &totalCompressedSize = mTotCompressedSize,
                    &totalUncompressedSize = mTotUncompressedSize]() -> arrow::Future<std::shared_ptr<arrow::RecordBatch>> {
    using namespace ROOT::Experimental;
    std::vector<std::shared_ptr<arrow::Array>> columns;
    std::vector<std::shared_ptr<arrow::Field>> fields = dataset_schema->fields();

    auto containerFS = std::dynamic_pointer_cast<VirtualRootFileSystemBase>(ntupleFragment->source().filesystem());
    auto fs = std::dynamic_pointer_cast<RNTupleFileSystem>(containerFS->GetSubFilesystem(ntupleFragment->source()));

    int64_t rows = -1;
    ROOT::Experimental::RNTuple* rntuple = fs->GetRNTuple(ntupleFragment->source());
    auto reader = ROOT::Experimental::RNTupleReader::Open(rntuple);
    auto& model = reader->GetModel();
    for (auto& physicalField : fields) {
      auto bulk = model.CreateBulk(physicalField->name());

      auto listType = std::dynamic_pointer_cast<arrow::FixedSizeListType>(physicalField->type());

      auto& descriptor = reader->GetDescriptor();
      auto totalEntries = reader->GetNEntries();

      if (rows == -1) {
        rows = totalEntries;
      }
      if (rows != totalEntries) {
        throw runtime_error_f("Unmatching number of rows for branch %s", physicalField->name().c_str());
      }
      arrow::Status status;
      int readEntries = 0;
      std::shared_ptr<arrow::Array> array;
      if (physicalField->type() == arrow::boolean() ||
          (listType && physicalField->type()->field(0)->type() == arrow::boolean())) {
        if (listType) {
          std::unique_ptr<arrow::ArrayBuilder> builder = nullptr;
          auto status = arrow::MakeBuilder(pool, physicalField->type()->field(0)->type(), &builder);
          if (!status.ok()) {
            throw runtime_error("Cannot create value builder");
          }
          auto listBuilder = std::make_unique<arrow::FixedSizeListBuilder>(pool, std::move(builder), listType->list_size());
          auto valueBuilder = listBuilder.get()->value_builder();
          // boolean array special case: we need to use builder to create the bitmap
          status = valueBuilder->Reserve(totalEntries * listType->list_size());
          status &= listBuilder->Reserve(totalEntries);
          if (!status.ok()) {
            throw runtime_error("Failed to reserve memory for array builder");
          }
          auto clusterIt = descriptor.FindClusterId(0, 0);
          // No adoption for now...
          // bulk.AdoptBuffer(buffer, totalEntries)
          while (clusterIt != kInvalidDescriptorId) {
            auto& index = descriptor.GetClusterDescriptor(clusterIt);
            auto mask = std::make_unique<bool[]>(index.GetNEntries());
            std::fill(mask.get(), mask.get() + index.GetNEntries(), true);
            void* ptr = bulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries());
            int readLast = index.GetNEntries();
            readEntries += readLast;
            status &= static_cast<arrow::BooleanBuilder*>(valueBuilder)->AppendValues(reinterpret_cast<uint8_t const*>(ptr), readLast * listType->list_size());
            clusterIt = descriptor.FindNextClusterId(clusterIt);
          }
          status &= static_cast<arrow::FixedSizeListBuilder*>(listBuilder.get())->AppendValues(readEntries);
          if (!status.ok()) {
            throw runtime_error("Failed to append values to array");
          }
          status &= listBuilder->Finish(&array);
          if (!status.ok()) {
            throw runtime_error("Failed to create array");
          }
        } else if (listType == nullptr) {
          std::unique_ptr<arrow::ArrayBuilder> builder = nullptr;
          auto status = arrow::MakeBuilder(pool, physicalField->type(), &builder);
          if (!status.ok()) {
            throw runtime_error("Cannot create builder");
          }
          auto valueBuilder = static_cast<arrow::BooleanBuilder*>(builder.get());
          // boolean array special case: we need to use builder to create the bitmap
          status = valueBuilder->Reserve(totalEntries);
          if (!status.ok()) {
            throw runtime_error("Failed to reserve memory for array builder");
          }
          auto clusterIt = descriptor.FindClusterId(0, 0);
          while (clusterIt != kInvalidDescriptorId) {
            auto& index = descriptor.GetClusterDescriptor(clusterIt);
            auto mask = std::make_unique<bool[]>(index.GetNEntries());
            std::fill(mask.get(), mask.get() + index.GetNEntries(), true);
            void* ptr = bulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries());
            int readLast = index.GetNEntries();
            readEntries += readLast;
            status &= valueBuilder->AppendValues(reinterpret_cast<uint8_t const*>(ptr), readLast);
            clusterIt = descriptor.FindNextClusterId(clusterIt);
          }
          if (!status.ok()) {
            throw runtime_error("Failed to append values to array");
          }
          status &= valueBuilder->Finish(&array);
          if (!status.ok()) {
            throw runtime_error("Failed to create array");
          }
        }
      } else {
        // other types: use serialized read to build arrays directly.
        auto typeSize = physicalField->type()->byte_width();
        // FIXME: for now...
        auto bytes = 0;
        auto branchSize = bytes ? bytes : 1000000;
        auto&& result = arrow::AllocateResizableBuffer(branchSize, pool);
        if (!result.ok()) {
          throw runtime_error("Cannot allocate values buffer");
        }
        std::shared_ptr<arrow::Buffer> arrowValuesBuffer = std::move(result).ValueUnsafe();
        auto ptr = arrowValuesBuffer->mutable_data();
        if (ptr == nullptr) {
          throw runtime_error("Invalid buffer");
        }

        std::unique_ptr<TBufferFile> offsetBuffer = nullptr;

        std::shared_ptr<arrow::Buffer> arrowOffsetBuffer;
        std::span<int> offsets;
        int size = 0;
        uint32_t totalSize = 0;
        int64_t listSize = 1;
        if (auto fixedSizeList = std::dynamic_pointer_cast<arrow::FixedSizeListType>(physicalField->type())) {
          listSize = fixedSizeList->list_size();
          typeSize = fixedSizeList->field(0)->type()->byte_width();
          auto clusterIt = descriptor.FindClusterId(0, 0);
          while (clusterIt != kInvalidDescriptorId) {
            auto& index = descriptor.GetClusterDescriptor(clusterIt);
            auto mask = std::make_unique<bool[]>(index.GetNEntries());
            std::fill(mask.get(), mask.get() + index.GetNEntries(), true);
            void* inPtr = bulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries());

            int readLast = index.GetNEntries();
            if (listSize == -1) {
              size = offsets[readEntries + readLast] - offsets[readEntries];
            } else {
              size = readLast * listSize;
            }
            readEntries += readLast;
            memcpy(ptr, inPtr, size * typeSize);
            ptr += (ptrdiff_t)(size * typeSize);
            clusterIt = descriptor.FindNextClusterId(clusterIt);
          }
        } else if (auto vlaListType = std::dynamic_pointer_cast<arrow::ListType>(physicalField->type())) {
          listSize = -1;
          typeSize = vlaListType->field(0)->type()->byte_width();
          offsetBuffer = std::make_unique<TBufferFile>(TBuffer::EMode::kWrite, 4 * 1024 * 1024);
          result = arrow::AllocateResizableBuffer((totalEntries + 1) * (int64_t)sizeof(int), pool);
          if (!result.ok()) {
            throw runtime_error("Cannot allocate offset buffer");
          }
          arrowOffsetBuffer = std::move(result).ValueUnsafe();

          // Offset bulk
          auto offsetBulk = model.CreateBulk(physicalField->name());
          // Actual values are in a different place...
          bulk = model.CreateBulk(physicalField->name());
          auto clusterIt = descriptor.FindClusterId(0, 0);
          auto* ptrOffset = reinterpret_cast<int*>(arrowOffsetBuffer->mutable_data());
          auto* tPtrOffset = reinterpret_cast<int*>(ptrOffset);
          offsets = std::span<int>{tPtrOffset, tPtrOffset + totalEntries + 1};

          auto copyOffsets = [&arrowValuesBuffer, &pool, &ptrOffset, &ptr, &totalSize](auto inPtr, size_t total) {
            using value_type = typename std::decay_t<decltype(*inPtr)>::value_type;
            for (size_t i = 0; i < total; i++) {
              *ptrOffset++ = totalSize;
              totalSize += inPtr[i].size();
            }
            *ptrOffset = totalSize;
            auto&& result = arrow::AllocateResizableBuffer(totalSize * sizeof(value_type), pool);
            if (!result.ok()) {
              throw runtime_error("Cannot allocate values buffer");
            }
            arrowValuesBuffer = std::move(result).ValueUnsafe();
            ptr = (uint8_t*)(arrowValuesBuffer->mutable_data());
            // Calculate the size of the buffer here.
            for (size_t i = 0; i < total; i++) {
              int vlaSizeInBytes = inPtr[i].size() * sizeof(value_type);
              if (vlaSizeInBytes == 0) {
                continue;
              }
              memcpy(ptr, inPtr[i].data(), vlaSizeInBytes);
              ptr += vlaSizeInBytes;
            }
          };

          while (clusterIt != kInvalidDescriptorId) {
            auto& index = descriptor.GetClusterDescriptor(clusterIt);
            auto mask = std::make_unique<bool[]>(index.GetNEntries());
            std::fill(mask.get(), mask.get() + index.GetNEntries(), true);
            int readLast = index.GetNEntries();
            switch (vlaListType->field(0)->type()->id()) {
              case arrow::Type::FLOAT: {
                copyOffsets((ROOT::Internal::VecOps::RVec<float>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              case arrow::Type::DOUBLE: {
                copyOffsets((ROOT::Internal::VecOps::RVec<double>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              case arrow::Type::INT8: {
                copyOffsets((ROOT::Internal::VecOps::RVec<int8_t>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              case arrow::Type::INT16: {
                copyOffsets((ROOT::Internal::VecOps::RVec<int16_t>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              case arrow::Type::INT32: {
                copyOffsets((ROOT::Internal::VecOps::RVec<int32_t>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              case arrow::Type::INT64: {
                copyOffsets((ROOT::Internal::VecOps::RVec<int64_t>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              case arrow::Type::UINT8: {
                copyOffsets((ROOT::Internal::VecOps::RVec<uint8_t>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              case arrow::Type::UINT16: {
                copyOffsets((ROOT::Internal::VecOps::RVec<uint16_t>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              case arrow::Type::UINT32: {
                copyOffsets((ROOT::Internal::VecOps::RVec<uint32_t>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              case arrow::Type::UINT64: {
                copyOffsets((ROOT::Internal::VecOps::RVec<uint64_t>*)offsetBulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries()), readLast);
              } break;
              default: {
                throw runtime_error("Unsupported kind of VLA");
              } break;
            }

            readEntries += readLast;
            clusterIt = descriptor.FindNextClusterId(clusterIt);
          }
        } else {
          auto clusterIt = descriptor.FindClusterId(0, 0);
          while (clusterIt != kInvalidDescriptorId) {
            auto& index = descriptor.GetClusterDescriptor(clusterIt);
            auto mask = std::make_unique<bool[]>(index.GetNEntries());
            std::fill(mask.get(), mask.get() + index.GetNEntries(), true);
            void* inPtr = bulk.ReadBulk(RClusterIndex(clusterIt, index.GetFirstEntryIndex()), mask.get(), index.GetNEntries());

            int readLast = index.GetNEntries();
            if (listSize == -1) {
              size = offsets[readEntries + readLast] - offsets[readEntries];
            } else {
              size = readLast * listSize;
            }
            readEntries += readLast;
            memcpy(ptr, inPtr, size * typeSize);
            ptr += (ptrdiff_t)(size * typeSize);
            clusterIt = descriptor.FindNextClusterId(clusterIt);
          }
        }
        switch (listSize) {
          case -1: {
            auto varray = std::make_shared<arrow::PrimitiveArray>(physicalField->type()->field(0)->type(), totalSize, arrowValuesBuffer);
            array = std::make_shared<arrow::ListArray>(physicalField->type(), readEntries, arrowOffsetBuffer, varray);
          } break;
          case 1: {
            totalSize = readEntries * listSize;
            array = std::make_shared<arrow::PrimitiveArray>(physicalField->type(), readEntries, arrowValuesBuffer);

          } break;
          default: {
            totalSize = readEntries * listSize;
            auto varray = std::make_shared<arrow::PrimitiveArray>(physicalField->type()->field(0)->type(), totalSize, arrowValuesBuffer);
            array = std::make_shared<arrow::FixedSizeListArray>(physicalField->type(), readEntries, varray);
          }
        }
      }
      columns.push_back(array);
    }

    auto batch = arrow::RecordBatch::Make(dataset_schema, rows, columns);
    return batch;
  };

  return generator;
}

arrow::Result<std::shared_ptr<arrow::io::OutputStream>> TTreeFileSystem::OpenOutputStream(
  const std::string& path,
  const std::shared_ptr<const arrow::KeyValueMetadata>& metadata)
{
  arrow::dataset::FileSource source{path, shared_from_this()};
  auto prefix = metadata->Get("branch_prefix");
  if (prefix.ok()) {
    return std::make_shared<TTreeOutputStream>(GetTree(source), *prefix);
  }
  return std::make_shared<TTreeOutputStream>(GetTree(source), "");
}

std::shared_ptr<arrow::dataset::FileWriteOptions>
  RNTupleFileFormat::DefaultWriteOptions()
{
  return std::make_shared<RNTupleFileWriteOptions>(shared_from_this());
}

arrow::Result<std::shared_ptr<arrow::dataset::FileWriter>> RNTupleFileFormat::MakeWriter(std::shared_ptr<arrow::io::OutputStream> destination,
                                                                                         std::shared_ptr<arrow::Schema> schema,
                                                                                         std::shared_ptr<arrow::dataset::FileWriteOptions> options,
                                                                                         arrow::fs::FileLocator destination_locator) const
{
  auto writer = std::make_shared<RNTupleFileWriter>(schema, options, destination, destination_locator);
  return std::dynamic_pointer_cast<arrow::dataset::FileWriter>(writer);
}

TBufferFileFS::TBufferFileFS(TBufferFile* f)
  : VirtualRootFileSystemBase(),
    mBuffer(f),
    mFilesystem(nullptr)
{
}

TTreeFileSystem::~TTreeFileSystem() = default;

RNTupleFileSystem::~RNTupleFileSystem() = default;

arrow::Result<arrow::fs::FileInfo> TBufferFileFS::GetFileInfo(const std::string& path)
{
  arrow::fs::FileInfo result;
  result.set_type(arrow::fs::FileType::NotFound);
  result.set_path(path);
  arrow::dataset::FileSource source(path, shared_from_this());

  // Only once to avoid rereading the streamed tree.
  if (!mFilesystem.get()) {
    return result;
  }

  // For now we only support single trees.
  if (std::dynamic_pointer_cast<SingleTreeFileSystem>(mFilesystem)) {
    result.set_type(arrow::fs::FileType::File);
    return result;
  }
  if (std::dynamic_pointer_cast<SingleRNTupleFileSystem>(mFilesystem)) {
    result.set_type(arrow::fs::FileType::File);
    return result;
  }
  return result;
}

std::shared_ptr<VirtualRootFileSystemBase> TBufferFileFS::GetSubFilesystem(arrow::dataset::FileSource source)
{
  if (!mFilesystem.get()) {
    auto tree = ((TTree*)mBuffer->ReadObject(TTree::Class()));
    mFilesystem = std::make_shared<SingleTreeFileSystem>(tree);
  }
  return mFilesystem;
}
} // namespace o2::framework

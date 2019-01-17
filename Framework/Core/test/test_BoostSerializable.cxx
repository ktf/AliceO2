#include <type_traits>
#include <array>
#include <iostream>
#include <string>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

// c++ -std=c++11 -isystem /usr/local/include test_boostSerializable.cxx

template <typename...>
using void_t = void;

//Base case called by all overloads when needed. Derives from false_type.
template <typename Type, typename Archive = boost::archive::binary_oarchive, typename = void_t<>, int = 0>
struct is_boost_serializable_base : std::false_type {
};

template <class Type, typename Archive, int V>
struct is_boost_serializable_base<Type, Archive, void_t<decltype(std::declval<Type&>().serialize(std::declval<Archive&>(), 0))>, V>
  : std::true_type {
};

template <class Type, typename Archive, long V>
struct is_boost_serializable_base<Type, Archive, void_t<decltype(&boost::serialization::serialize<Archive&, Type&>)>, V>
  : std::true_type {
};

//Check if provided type implements a boost serialize method directly
// template <class Type, typename Archive>
// struct is_boost_serializable_base<Type, Archive, void_t<decltype(std::declval<Type&>().serialize(std::declval<Archive&>(), 0)), decltype(boost::serialization::serialize(std::declval<Archive&>(), std::declval<Type&>(), 0))>>
//   : std::true_type {
// };

// template <class Type, typename Archive>
// struct is_boost_serializable_base<Type, Archive, void_t<decltype(std::declval<Type&>().serialize(std::declval<Archive&>(), 0))>>
//   : std::true_type {
// };

//Base implementation to provided recurrence. Wraps around base templates
template <class Type, typename Archive = boost::archive::binary_oarchive, typename = void_t<>>
struct is_boost_serializable
  : is_boost_serializable_base<Type, Archive, void_t<>, 0> {
};

//Call base implementation in contained class/type if possible
template <class Type, typename Archive>
struct is_boost_serializable<Type, Archive, void_t<typename Type::value_type>>
  : is_boost_serializable<typename Type::value_type, Archive, void_t<typename Type::value_type>> {
};

//Call base implementation in contained class/type if possible. Added default archive type for convenience
template <class Type>
struct is_boost_serializable<Type, boost::archive::binary_oarchive, void_t<typename Type::value_type>>
  : is_boost_serializable<typename Type::value_type, boost::archive::binary_oarchive> {
};

namespace o2
{
namespace mid
{
/// Column data structure for MID
struct ColumnData {
  int deId;                     ///< Index of the detection element
  int columnId;                 ///< Column in DE
  std::array<uint, 5> patterns; ///< patterns

};

struct ColumnDataNoBoost {
  int deId;                     ///< Index of the detection element
  int columnId;                 ///< Column in DE
  std::array<uint, 5> patterns; ///< patterns
  private:
  void *foo;
};

struct ColumnDataIntrusive {
  int deId;                     ///< Index of the detection element
  int columnId;                 ///< Column in DE
  std::array<uint, 5> patterns; ///< patterns

  friend class boost::serialization::access;

  /// Serializes the struct
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    ar& deId;
    ar& columnId;
    ar& patterns;
  }

};
} // namespace mid
} // namespace o2

namespace boost
{
namespace serialization
{

/// Serializes the struct
template <class Archive>
void serialize(Archive& ar, o2::mid::ColumnData& data, const unsigned int version)
{
  ar& data.deId;
  ar& data.columnId;
  ar& data.patterns;
}
} // namespace serialization
} // namespace boost

int main() {
  if constexpr ( is_boost_serializable<o2::mid::ColumnData>::value == true ) {
    std::cout << "ColumnData is serializable!\n";
  }

  if constexpr ( is_boost_serializable<o2::mid::ColumnDataIntrusive>::value == true ) {
    std::cout << "ColumnDataIntrusive is serializable!\n";
  }

  if constexpr ( is_boost_serializable<o2::mid::ColumnDataNoBoost>::value == true ) {
    std::cout << "ColumnDataNoBoost is serializable!\n";
  }

  return 0;
}


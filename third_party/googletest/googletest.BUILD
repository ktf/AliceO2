licenses(["notice"])

package(default_visibility = ["//visibility:public"])

install_script = "\n".join([
    "cd ../../../",
    "pwd",
    "cd external/com_github_google_googletest",
    "cmake .",
    "cd ../..",
    "rm -rf ../../$(@D)/*",
    "cp -R ./external/googletest/* $(@D)",
])

genrule(
    name = "googletest",
    outs = ["gtest"],
    executable = 1,
    cmd = install_script,
)

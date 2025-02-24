export TERM=${TERM:="xterm-256color"}


PICO_SDK_VERSION="2.1.1"
PIMORONI_PICO_VERSION="feature/picovector2-and-layers"


function log_success {
	echo -e "$(tput setaf 2)$1$(tput sgr0)"
}

function log_inform {
	echo -e "$(tput setaf 6)$1$(tput sgr0)"
}

function log_warning {
	echo -e "$(tput setaf 1)$1$(tput sgr0)"
}

function ci_clone {
    NAME=$1
    USER=$2
    REPO=$3
    BRANCH=$4
    log_inform "Cloning $NAME $USER/$REPO/$BRANCH"
    git clone https://github.com/$USER/$REPO "$CI_BUILD_ROOT/$REPO"
    cd "$CI_BUILD_ROOT/$REPO" || return 1
    git checkout $BRANCH
    git submodule update --init
    cd "$CI_BUILD_ROOT"
}

function ci_apt_install_build_deps {
    sudo apt update && sudo apt install ccache
}

function ci_prepare_all {
    mkdir -p $CI_BUILD_ROOT
    ci_clone "Pimoroni Pico" "pimoroni" "pimoroni-pico" "$PIMORONI_PICO_VERSION"
    ci_clone "Pico SDK" "raspberrypi" "pico-sdk" "$PICO_SDK_VERSION"
    ci_clone "Pico Extras" "raspberrypi" "pico-extras" "sdk-$PICO_SDK_VERSION"
}

function ci_debug {
    log_inform "Project root: $CI_PROJECT_ROOT"
    log_inform "Build root: $CI_BUILD_ROOT"
}

function ci_cmake_configure {
    PIMORONI_PICO_PATH="$CI_BUILD_ROOT/pimoroni-pico"
    PICO_SDK_PATH="$CI_BUILD_ROOT/pico-sdk"
    CMAKE_INSTALL_PREFIX="$CI_BUILD_ROOT/out"
    TOOLS_DIR="$CI_BUILD_ROOT/tools"

    cmake $CI_PROJECT_ROOT -B "$CI_BUILD_ROOT" \
    -DPICOTOOL_FORCE_FETCH_FROM_GIT=1 \
    -DPICOTOOL_FETCH_FROM_GIT_PATH="$TOOLS_DIR/picotool" \
    -DPICO_BUILD_DOCS=0 \
    -DCMAKE_BUILD_TYPE=$CI_BUILD_TYPE \
    -DPIMORONI_PICO_PATH=$PIMORONI_PICO_PATH \
    -DPICO_SDK_PATH=$PICO_SDK_PATH \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
}

function ci_cmake_build {
    ccache --zero-stats || true
    cmake --build $CI_BUILD_ROOT -j 2
    ccache --show-stats || true
}

function ci_cmake_package {
    cmake --build $CI_BUILD_ROOT --target package -j 2
}

if [ -z ${CI_USE_ENV+x} ] || [ -z ${CI_PROJECT_ROOT+x} ] || [ -z ${CI_BUILD_ROOT+x} ]; then
    SCRIPT_PATH="$(dirname $0)"
    CI_PROJECT_ROOT=$(realpath "$SCRIPT_PATH/..")
    CI_BUILD_ROOT=$(pwd)
fi

if [ -z ${CI_BUILD_TYPE+x} ]; then
    CI_BUILD_TYPE="Release"
fi

ci_debug
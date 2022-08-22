cmake_minimum_required(VERSION 3.17)
project(TOAD)

set(CMAKE_CXX_STANDARD 17)

INCLUDE_DIRECTORIES(/mnt/data_server/hoeller/toad3/code/libfst/share/include)

add_executable(toad main.cpp
        htnModel/Model.cpp htnModel/ProgressionNetwork.cpp
        translation/CFGtoFDAtranslator.cpp
        translation/CFtoRegGrammarEnc.cpp
        translation/StateBasedReachability.cpp
        translation/HeuFaDist.cpp
        utils/IntUtil.cpp utils/noDelIntSet.cpp utils/IntPairHeap.cpp utils/IntStack.cpp utils/StringUtil.cpp
        ModelWriter.cpp SASWriter.cpp)

target_link_libraries (toad /mnt/data_server/hoeller/toad3/code/libfst/openfst-1.8.2/src/lib/.libs/libfst.so)
#target_link_libraries (toad libfst.so)

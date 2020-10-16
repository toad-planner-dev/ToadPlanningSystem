cmake_minimum_required(VERSION 3.17)
project(TOStripsApproximation)

set(CMAKE_CXX_STANDARD 14)

add_executable(TOStripsApproximation main.cpp htnModel/Model.cpp htnModel/ProgressionNetwork.cpp intDataStructures/IntUtil.cpp intDataStructures/StringUtil.cpp translation/CFGtoFDAtranslator.cpp translation/CFGtoFDAtranslator.h translation/FiniteAutomaton.cpp translation/FiniteAutomaton.h translation/Pair.cpp translation/Pair.h ModelWriter.cpp ModelWriter.h)

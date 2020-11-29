cmake_minimum_required(VERSION 3.17)
project(TOAD)

set(CMAKE_CXX_STANDARD 14)

add_executable(toad main.cpp
        htnModel/Model.cpp htnModel/ProgressionNetwork.cpp
        translation/CFGtoFDAtranslator.cpp translation/CFGtoFDAtranslator.h
        translation/FiniteAutomaton.cpp translation/FiniteAutomaton.h translation/Pair.cpp translation/Pair.h
        optimization/RPGReachability.cpp optimization/RPGReachability.h
        ModelWriter.cpp ModelWriter.h
        utils/IntUtil.cpp utils/noDelIntSet.cpp utils/IntPairHeap.cpp utils/IntStack.cpp utils/StringUtil.cpp
        verification/GroundVerifier.cpp verification/GroundVerifier.h translation/TailRecAnalysis.cpp translation/TailRecAnalysis.h optimization/DFAMinimization.cpp optimization/DFAMinimization.h translation/CFtoRegGrammarEnc.cpp translation/CFtoRegGrammarEnc.h ChainWriter.cpp ChainWriter.h macroAction.cpp macroAction.h translation/UnderApproximation.cpp translation/UnderApproximation.h utils/TripleSet.cpp utils/TripleSet.h BinSetWriter.cpp BinSetWriter.h)

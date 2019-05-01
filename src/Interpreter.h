/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2019, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Interpreter.h
 *
 * Declares the interpreter class for executing RAM programs.
 *
 ***********************************************************************/

#pragma once

#include "InterpreterContext.h"
#include "InterpreterRelation.h"
#include "LVMCode.h"
#include "LVMGenerator.h"
#include "Logger.h"
#include "RamTranslationUnit.h"
#include "RamTypes.h"
#include "RelationRepresentation.h"

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <dlfcn.h>

#define SOUFFLE_DLL "libfunctors.so"

namespace souffle {

class InterpreterProgInterface;

/**
 * Interpreter Interface
 */
class Interpreter {
public:
    Interpreter(RamTranslationUnit& tUnit) : translationUnit(tUnit) {}

    virtual ~Interpreter() {
        for (auto& x : environment) {
            delete x.second;
        }
    }

    /** Get translation unit */
    RamTranslationUnit& getTranslationUnit() {
        return translationUnit;
    }

    /** Interface for executing the main program */
    virtual void executeMain() = 0;

    /** Execute the subroutine */
    virtual void executeSubroutine(const std::string& name, const std::vector<RamDomain>& arguments,
            std::vector<RamDomain>& returnValues, std::vector<bool>& returnErrors) = 0;

protected:
    friend InterpreterProgInterface;

    /** relation environment type */
    using relation_map = std::map<std::string, InterpreterRelation*>;

    /** Get symbol table */
    SymbolTable& getSymbolTable() {
        return translationUnit.getSymbolTable();
    }

    /** Get relation map */
    relation_map& getRelationMap() const {
        return const_cast<relation_map&>(environment);
    }

    /** Get Counter */
    int getCounter() {
        return counter;
    }

    /** Increment counter */
    int incCounter() {
        return counter++;
    }

    /** Increment iteration number */
    void incIterationNumber() {
        iteration++;
    }

    /** Get Iteration Number */
    size_t getIterationNumber() const {
        return iteration;
    }
    /** Reset iteration number */
    void resetIterationNumber() {
        iteration = 0;
    }

    /** TODO not implemented yet */
    void createRelation(const RamRelation& id) {}

    /** Get relation */
    InterpreterRelation& getRelation(const std::string& name) {
        // look up relation
        auto pos = environment.find(name);
        assert(pos != environment.end());
        return *pos->second;
    }

    /** Get relation */
    inline InterpreterRelation& getRelation(const RamRelation& id) {
        return getRelation(id.getName());
    }

    /** Drop relation */
    void dropRelation(const RamRelation& id) {
        InterpreterRelation& rel = getRelation(id);
		environment.erase(id.getName());
        delete &rel;
    }

    /** Drop relation */
    void dropRelation(const std::string& relName) {
        InterpreterRelation& rel = getRelation(relName);
        environment.erase(relName);
        delete &rel;
    }

    /** Swap relation */
    void swapRelation(const RamRelation& ramRel1, const RamRelation& ramRel2) {
        InterpreterRelation* rel1 = &getRelation(ramRel1);
        InterpreterRelation* rel2 = &getRelation(ramRel2);
        environment[ramRel1.getName()] = rel2;
        environment[ramRel2.getName()] = rel1;
    }

    /** Swap relation */
    void swapRelation(const std::string& ramRel1, const std::string& ramRel2) {
        InterpreterRelation* rel1 = &getRelation(ramRel1);
        InterpreterRelation* rel2 = &getRelation(ramRel2);
        environment[ramRel1] = rel2;
        environment[ramRel2] = rel1;
    }

    /** load dll */
    void* loadDLL() {
        if (dll == nullptr) {
            // check environment variable
            std::string fname = SOUFFLE_DLL;
            dll = dlopen(SOUFFLE_DLL, RTLD_LAZY);
            if (dll == nullptr) {
                std::cerr << "Cannot find Souffle's DLL" << std::endl;
                exit(1);
            }
        }
        return dll;
    }

    // Lookup for IndexScan iter, resize the vector if idx > size */
    std::pair<index_set::iterator, index_set::iterator>& lookUpIndexScanIterator(size_t idx) {
        if (idx >= indexScanIteratorPool.size()) {
            indexScanIteratorPool.resize((idx + 1) * 2);
        }
        return indexScanIteratorPool[idx];
    }

    // Lookup for IndexChoice iter, resize the vector if idx > size */
    std::pair<index_set::iterator, index_set::iterator>& lookUpIndexChoiceIterator(size_t idx) {
        if (idx >= indexChoiceIteratorPool.size()) {
            indexChoiceIteratorPool.resize((idx + 1) * 2);
        }
        return indexChoiceIteratorPool[idx];
    }

    /** Lookup for Scan iter, resize the vector if idx > size */
    std::pair<InterpreterRelation::iterator, InterpreterRelation::iterator>& lookUpScanIterator(size_t idx) {
        if (idx >= scanIteratorPool.size()) {
            scanIteratorPool.resize((idx + 1) * 2);
        }
        return scanIteratorPool[idx];
    }

    /** Lookup for Choice iter, resize the vector if idx > size */
    std::pair<InterpreterRelation::iterator, InterpreterRelation::iterator>& lookUpChoiceIterator(
            size_t idx) {
        if (idx >= choiceIteratorPool.size()) {
            choiceIteratorPool.resize((idx + 1) * 2);
        }
        return choiceIteratorPool[idx];
    }

private:
    friend InterpreterProgInterface;

    /** RAM translation Unit */
    RamTranslationUnit& translationUnit;

    /** Relation Environment */
    relation_map environment;

    /** Value stack */
    std::stack<RamDomain> stack;

    /** counters for atom profiling */
    std::map<std::string, std::map<size_t, size_t>> frequencies;

    /** counters for non-existence check */
    std::map<std::string, std::atomic<size_t>> reads;

    /** List of loggers for logtimer */
    std::vector<Logger*> timers;

    /** counter for $ operator */
    int counter = 0;

    /** iteration number (in a fix-point calculation) */
    size_t iteration = 0;

    /** Dynamic library for user-defined functors */
    void* dll = nullptr;

    /** List of iters for IndexScan operation */
    std::vector<std::pair<index_set::iterator, index_set::iterator>> indexScanIteratorPool;

    /** List of iters for IndexChoice operation */
    std::vector<std::pair<index_set::iterator, index_set::iterator>> indexChoiceIteratorPool;

    /** List of iters for Scan operation */
    std::vector<std::pair<InterpreterRelation::iterator, InterpreterRelation::iterator>> scanIteratorPool;

    /** List of iters for Choice operation */
    std::vector<std::pair<InterpreterRelation::iterator, InterpreterRelation::iterator>> choiceIteratorPool;

    /** stratum */
    size_t level = 0;
};

}  // end of namespace souffle

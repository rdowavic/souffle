/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file SumTypeBranches.h
 *
 * A wrapper for calculating a mapping between branches and types that declare them.
 *
 ***********************************************************************/

#pragma once

#include "ast/TypeSystem.h"
#include "ast/analysis/Analysis.h"

namespace souffle {

class AstTranslationUnit;

class SumTypeBranchesAnalysis : public AstAnalysis {
public:
    static constexpr const char* name = "sum-type-branches";

    SumTypeBranchesAnalysis() : AstAnalysis(name) {}

    void run(const AstTranslationUnit& translationUnit) override;

    /**
     * A type can be nullptr in case of malformed program.
     */
    const Type* getType(const std::string& branch) const {
        if (contains(branchToType, branch)) {
            return branchToType.at(branch);
        } else {
            return nullptr;
        }
    }

private:
    std::map<std::string, const Type*> branchToType;
};

}  // namespace souffle

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cangjie/CHIR/CHIR.h"
#include "cangjie/CHIR/IR/CHIRBuilder.h"
#include "cangjie/Competition/ConstraintMatching/IntersectionMatching.h"
#include "cangjie/Competition/DominatorTree.h"
#include "cangjie/Competition/Phi.h"
#include "cangjie/Competition/SSABuilder.h"

namespace {

using namespace Cangjie::CHIR;
using namespace Competition;

// Minimal CHIR function builder shared by the CFG and pipeline tests.
struct TestFunction {
    std::unordered_map<unsigned int, std::string> nameMap;
    CHIRContext context{&nameMap};
    CHIRBuilder builder{context};
    Function* function = nullptr;
    BlockGroup* body = nullptr;

    explicit TestFunction(bool withIntegerParameter = false)
    {
        builder.CreatePackage("default");
        std::vector<Type*> parameterTypes;
        if (withIntegerParameter) parameterTypes.push_back(builder.GetInt64Ty());
        auto* functionType = builder.GetType<FuncType>(
            parameterTypes, builder.GetUnitTy());
        function = builder.CreateFunction(functionType, "test_fn", "test_fn",
                                          "", "default");
        body = builder.CreateBlockGroup(*function);
        function->InitBody(*body);
    }

    Block* CreateBlock()
    {
        return builder.CreateBlock(body);
    }
};

struct Comparison {
    ExprKind kind;
    Matching::MatchedConstraints (*match)(
        Value*, const std::unordered_map<std::string, Alias>&);
    size_t trueCount;
    size_t falseCount;
};

// These checks cover SSA naming and constraint/value structure, not evaluation.
TEST(CompetitionSSATest, AliasAndPhiPreserveSSAIndices)
{
    // arrange
    Alias original("sample", "value", 12);
    Phi phi(Alias("sample", "value"), 2);

    // act
    auto roundTrip = Alias::from_string(original.to_string()).to_string();
    phi.setVarCounter(3);
    phi.setAliasCounterByIdx(0, 1);
    phi.setAliasCounterByIdx(1, 2);
    auto phiAliases = phi.getAliasesStrings();

    // assert
    EXPECT_EQ(roundTrip, "sample:value:12");
    EXPECT_EQ(phi.getVarString(), "sample:value:3");
    EXPECT_EQ(phiAliases,
              (std::vector<std::string>{"sample:value:1", "sample:value:2"}));
}

TEST(CompetitionSSATest, ConstraintDefinitionsAndUsesAreStructural)
{
    // arrange
    InitializationConstraint init("fn:x:0", 7);
    PhiConstraint phi("fn:x:1", {"fn:x:0", "fn:x:2"}, IVType);
    IntersectionConstraint intersection(
        "fn:x:2", "fn:x:1", Bound::minusInfinity(),
        IntersectionConstraint::Future{"fn:y:0", -1});
    AddConstraint add("fn:z:0", "fn:x:0", "fn:y:0");

    // act
    auto initUses = init.get_uses();
    auto phiUses = phi.get_uses();
    auto intersectionDef = intersection.get_def();
    auto intersectionUses = intersection.get_uses();
    auto addUses = add.get_uses();

    // assert
    EXPECT_EQ(init.get_def(), "fn:x:0");
    EXPECT_TRUE(initUses.empty());
    ASSERT_EQ(phiUses.size(), 2);
    EXPECT_EQ(phiUses[0].target_variable, "fn:x:0");
    EXPECT_EQ(phiUses[0].type, EdgeType::Data);
    EXPECT_EQ(intersectionDef, "fn:x:2");
    ASSERT_EQ(intersectionUses.size(), 2);
    EXPECT_EQ(intersectionUses[0].target_variable, "fn:x:1");
    EXPECT_EQ(intersectionUses[1].target_variable, "fn:y:0");
    EXPECT_EQ(intersectionUses[1].type, EdgeType::Future);
    ASSERT_EQ(addUses.size(), 2);
    EXPECT_EQ(addUses[0].target_variable, "fn:x:0");
    EXPECT_EQ(addUses[1].target_variable, "fn:y:0");
}

TEST(CompetitionSSATest, AbstractValuesExposeKindsAndBounds)
{
    // arrange
    IV integerValue;
    BV booleanValue;

    // act
    auto integerStartsAtBottom = integerValue.isBottom();
    integerValue.addConstant({2, 4});
    auto integerSetKind = integerValue.getKind();
    auto integerSetValues = integerValue.getValues();
    integerValue.setAsInterval(Bound::constant(2), Bound::constant(10), 2);
    booleanValue.addConstant({true});

    // assert
    EXPECT_TRUE(integerStartsAtBottom);
    EXPECT_EQ(integerSetKind, IV::Kind::Set);
    EXPECT_EQ(integerSetValues, (std::set<int>{2, 4}));
    EXPECT_EQ(integerValue.getKind(), IV::Kind::StridedInterval);
    EXPECT_EQ(integerValue.getLower(), Bound::constant(2));
    EXPECT_EQ(integerValue.getUpper(), Bound::constant(10));
    EXPECT_EQ(integerValue.getStride(), 2u);
    EXPECT_EQ(booleanValue.getKind(), BV::Kind::Set);
    EXPECT_EQ(booleanValue.getValues(), (std::set<bool>{true}));
}

TEST(CompetitionSSATest, DominatorsAndIDFOnDiamondCFG)
{
    // A diamond has a single join point in the iterated frontier of its left arm.
    // arrange
    TestFunction test;
    auto* entry = test.CreateBlock();
    auto* left = test.CreateBlock();
    auto* right = test.CreateBlock();
    auto* merge = test.CreateBlock();
    test.body->SetEntryBlock(entry);

    auto* condition = test.builder.CreateConstantExpression<BoolLiteral>(
        test.builder.GetBoolTy(), entry, true)->GetResult();
    entry->AppendExpression(
        test.builder.CreateTerminator<Branch>(condition, left, right, entry));
    left->AppendExpression(test.builder.CreateTerminator<GoTo>(merge, left));
    right->AppendExpression(test.builder.CreateTerminator<GoTo>(merge, right));
    merge->AppendExpression(test.builder.CreateTerminator<Exit>(merge));

    // act
    DominatorTree dominators(test.function);
    dominators.Compute();
    SSABuilder ssaBuilder(dominators);
    auto phiBlocks = ssaBuilder.PlacePhiNodes({left}, entry);

    // assert
    EXPECT_EQ(dominators.GetImmediateDominator(entry), entry);
    EXPECT_EQ(dominators.GetImmediateDominator(left), entry);
    EXPECT_EQ(dominators.GetImmediateDominator(right), entry);
    EXPECT_EQ(dominators.GetImmediateDominator(merge), entry);
    EXPECT_TRUE(dominators.Dominates(entry, merge));
    EXPECT_FALSE(dominators.Dominates(left, merge));
    ASSERT_EQ(phiBlocks.size(), 1);
    EXPECT_EQ(phiBlocks.front(), merge);
}

TEST(CompetitionSSATest, FullBranchPipelineExtractsRenamedConstraints)
{
    // Keep this order aligned with BuildDomTreeWithConstraints in production.
    // arrange
    TestFunction test(true);
    auto* entry = test.CreateBlock();
    auto* thenBlock = test.CreateBlock();
    auto* elseBlock = test.CreateBlock();
    auto* merge = test.CreateBlock();
    test.body->SetEntryBlock(entry);

    auto* x = test.builder.CreateParameter(test.builder.GetInt64Ty(),
                                           DebugLocation{}, *test.function);
    x->SetSrcCodeIdentifier("x");
    test.function->AddParam(*x);
    auto* constantExpression = test.builder.CreateConstantExpression<IntLiteral>(
        test.builder.GetInt64Ty(), entry, 5);
    entry->AppendExpression(constantExpression);
    auto* constant = constantExpression->GetResult();
    auto* conditionExpression = test.builder.CreateExpression<BinaryExpression>(
        test.builder.GetBoolTy(), ExprKind::LT, x, constant, entry);
    entry->AppendExpression(conditionExpression);
    auto* condition = conditionExpression->GetResult();

    entry->AppendExpression(test.builder.CreateTerminator<Branch>(
        condition, thenBlock, elseBlock, entry));
    thenBlock->AppendExpression(
        test.builder.CreateTerminator<GoTo>(merge, thenBlock));
    elseBlock->AppendExpression(
        test.builder.CreateTerminator<GoTo>(merge, elseBlock));
    merge->AppendExpression(test.builder.CreateTerminator<Exit>(merge));

    // act
    DominatorTree dominators(test.function);
    dominators.Compute();
    dominators.GenerateBranchConstraints();
    dominators.ConvertToSSA();
    dominators.GenerateSSAConstraints();
    dominators.DetectReturnValues();

    // assert
    auto& thenConstraints = dominators.GetBlockConstraints(thenBlock);
    auto& elseConstraints = dominators.GetBlockConstraints(elseBlock);
    ASSERT_EQ(thenConstraints.size(), 1);
    ASSERT_EQ(elseConstraints.size(), 1);
    auto thenIntersection =
        std::dynamic_pointer_cast<IntersectionConstraint>(thenConstraints[0]);
    auto elseIntersection =
        std::dynamic_pointer_cast<IntersectionConstraint>(elseConstraints[0]);
    ASSERT_NE(thenIntersection, nullptr);
    ASSERT_NE(elseIntersection, nullptr);
    EXPECT_EQ(thenIntersection->operand,
              dominators.GetFunctionUniqueName() + ":x:0");
    EXPECT_EQ(elseIntersection->operand,
              dominators.GetFunctionUniqueName() + ":x:0");
    EXPECT_EQ(thenIntersection->get_def(),
              dominators.GetFunctionUniqueName() + ":x:1");
    EXPECT_EQ(elseIntersection->get_def(),
              dominators.GetFunctionUniqueName() + ":x:2");

    bool foundInitialization = false;
    for (auto& constraint : dominators.GetBlockConstraints(entry)) {
        if (std::dynamic_pointer_cast<InitializationConstraint>(constraint)) {
            foundInitialization = true;
            break;
        }
    }
    EXPECT_TRUE(foundInitialization);
}

TEST(CompetitionSSATest, IntersectionMatchersCoverSixComparisons)
{
    // Check branch refinements directly, without invoking solver evaluation.
    // arrange
    TestFunction test;
    auto* block = test.CreateBlock();
    test.body->SetEntryBlock(block);
    auto* integerType = test.builder.GetInt64Ty();
    auto* boolType = test.builder.GetBoolTy();
    auto* functionType = test.builder.GetType<FuncType>(
        std::vector<Type*>{integerType, integerType}, test.builder.GetUnitTy());
    auto* function = test.builder.CreateFunction(functionType, "matcher_fn",
                                                "matcher_fn", "", "default");
    auto* x = test.builder.CreateParameter(integerType, DebugLocation{}, *function);
    auto* y = test.builder.CreateParameter(integerType, DebugLocation{}, *function);
    x->SetSrcCodeIdentifier("x");
    y->SetSrcCodeIdentifier("y");

    auto* constant = test.builder.CreateConstantExpression<IntLiteral>(
        integerType, block, 5)->GetResult();
    const std::unordered_map<std::string, Alias> aliases{
        {"x", Alias("matcher_fn", "x", 0)},
        {"y", Alias("matcher_fn", "y", 0)}};

    const std::vector<Comparison> comparisons{
        {ExprKind::LT, Matching::MatchLessThanConstraints, 1, 1},
        {ExprKind::GT, Matching::MatchGreaterThanConstraints, 1, 1},
        {ExprKind::EQUAL, Matching::MatchEqualConstraints, 1, 0},
        {ExprKind::NOTEQUAL, Matching::MatchNotEqualConstraints, 0, 1},
        {ExprKind::LE, Matching::MatchLessEqualConstraints, 1, 1},
        {ExprKind::GE, Matching::MatchGreaterEqualConstraints, 1, 1},
    };

    auto expectBounds = [](IntersectionConstraint* constraint, Bound lower,
                           Bound upper) {
        ASSERT_TRUE(std::holds_alternative<Bound>(constraint->lower_bound));
        ASSERT_TRUE(std::holds_alternative<Bound>(constraint->upper_bound));
        EXPECT_EQ(std::get<Bound>(constraint->lower_bound), lower);
        EXPECT_EQ(std::get<Bound>(constraint->upper_bound), upper);
    };

    for (const auto& comparison : comparisons) {
        // act
        auto* condition = test.builder.CreateExpression<BinaryExpression>(
            boolType, comparison.kind, x, constant, block)->GetResult();
        auto matched = comparison.match(condition, aliases);

        // assert
        EXPECT_EQ(matched.ifTrue.size(), comparison.trueCount);
        EXPECT_EQ(matched.ifFalse.size(), comparison.falseCount);
        switch (comparison.kind) {
            case ExprKind::LT:
                expectBounds(matched.ifTrue[0], Bound::minusInfinity(),
                             Bound::constant(4));
                expectBounds(matched.ifFalse[0], Bound::constant(5),
                             Bound::plusInfinity());
                break;
            case ExprKind::GT:
                expectBounds(matched.ifTrue[0], Bound::constant(6),
                             Bound::plusInfinity());
                expectBounds(matched.ifFalse[0], Bound::minusInfinity(),
                             Bound::constant(5));
                break;
            case ExprKind::EQUAL:
                expectBounds(matched.ifTrue[0], Bound::constant(5),
                             Bound::constant(5));
                break;
            case ExprKind::NOTEQUAL:
                expectBounds(matched.ifFalse[0], Bound::constant(5),
                             Bound::constant(5));
                break;
            case ExprKind::LE:
                expectBounds(matched.ifTrue[0], Bound::minusInfinity(),
                             Bound::constant(5));
                expectBounds(matched.ifFalse[0], Bound::constant(6),
                             Bound::plusInfinity());
                break;
            case ExprKind::GE:
                expectBounds(matched.ifTrue[0], Bound::constant(5),
                             Bound::plusInfinity());
                expectBounds(matched.ifFalse[0], Bound::minusInfinity(),
                             Bound::constant(4));
                break;
            default:
                FAIL() << "Unexpected comparison kind";
        }
        for (auto* constraint : matched.ifTrue) {
            EXPECT_EQ(constraint->get_def(), "x");
            EXPECT_EQ(constraint->operand, "x");
            delete constraint;
        }
        for (auto* constraint : matched.ifFalse) {
            EXPECT_EQ(constraint->get_def(), "x");
            EXPECT_EQ(constraint->operand, "x");
            delete constraint;
        }
    }

    const std::vector<Comparison> variableComparisons{
        {ExprKind::LT, Matching::MatchLessThanConstraints, 2, 2},
        {ExprKind::GT, Matching::MatchGreaterThanConstraints, 2, 2},
        {ExprKind::EQUAL, Matching::MatchEqualConstraints, 2, 0},
        {ExprKind::NOTEQUAL, Matching::MatchNotEqualConstraints, 0, 2},
        {ExprKind::LE, Matching::MatchLessEqualConstraints, 2, 2},
        {ExprKind::GE, Matching::MatchGreaterEqualConstraints, 2, 2},
    };

    for (const auto& comparison : variableComparisons) {
        // act
        auto* condition = test.builder.CreateExpression<BinaryExpression>(
            boolType, comparison.kind, x, y, block)->GetResult();
        auto matched = comparison.match(condition, aliases);

        // assert
        EXPECT_EQ(matched.ifTrue.size(), comparison.trueCount);
        EXPECT_EQ(matched.ifFalse.size(), comparison.falseCount);
        for (auto* constraint : matched.ifTrue) {
            EXPECT_EQ(constraint->get_def(), constraint->operand);
            delete constraint;
        }
        for (auto* constraint : matched.ifFalse) {
            EXPECT_EQ(constraint->get_def(), constraint->operand);
            delete constraint;
        }
    }
}

}  // namespace
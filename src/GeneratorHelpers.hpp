#pragma once

#include <vector>
#include <string>

#include "EvaluatorEnums.hpp"
#include "TokenEnums.hpp"
#include "GeneratorHelpersEnums.hpp"

struct Token;
struct EvaluatorContext;
struct EvaluatorEnvironment;
struct GeneratorOutput;
struct StringOutput;
struct ObjectDefinition;

void StripInvocation(int& startTokenIndex, int& endTokenIndex);
int FindCloseParenTokenIndex(const std::vector<Token>& tokens, int startTokenIndex);

bool ExpectEvaluatorScope(const char* generatorName, const Token& token,
                          const EvaluatorContext& context, EvaluatorScope expectedScope);
bool IsForbiddenEvaluatorScope(const char* generatorName, const Token& token,
                               const EvaluatorContext& context, EvaluatorScope forbiddenScope);
bool ExpectTokenType(const char* generatorName, const Token& token, TokenType expectedType);
// Errors and returns false if out of invocation (or at closing paren)
bool ExpectInInvocation(const char* message, const std::vector<Token>& tokens, int indexToCheck,
                        int endInvocationIndex);

// Returns true if the symbol starts with :, &, or '
// TODO: Come up with better name
bool isSpecialSymbol(const Token& token);

// startTokenIndex should be the opening parenthesis of the array you want to retrieve arguments
// from. For example, you should pass in the opening paren of a function invocation to get its name
// as argument 0 and first arg as argument 1 This function would be simpler and faster if there was
// an actual syntax tree, because we wouldn't be repeatedly traversing all the arguments
// Returns -1 if argument is not within range
int getArgument(const std::vector<Token>& tokens, int startTokenIndex, int desiredArgumentIndex,
                int endTokenIndex);
int getExpectedArgument(const char* message, const std::vector<Token>& tokens, int startTokenIndex,
                        int desiredArgumentIndex, int endTokenIndex);
// Expects startTokenIndex to be the invocation. The name of the invocation is included in the count
// Note: Body arguments will not work properly with this
int getNumArguments(const std::vector<Token>& tokens, int startTokenIndex, int endTokenIndex);
// Like getNumArguments, includes invocation
bool ExpectNumArguments(const std::vector<Token>& tokens, int startTokenIndex, int endTokenIndex,
                        int numExpectedArguments);
bool isLastArgument(const std::vector<Token>& tokens, int startTokenIndex, int endTokenIndex);
// There are no more arguments once this returns endArrayTokenIndex
int getNextArgument(const std::vector<Token>& tokens, int currentTokenIndex,
                    int endArrayTokenIndex);

// If the current token is a scope, skip it. This is useful when a generator has already opened a
// block, so it knows the scope comes from the generator invocation
int blockAbsorbScope(const std::vector<Token>& tokens, int startBlockIndex);

const Token* FindTokenExpressionEnd(const Token* startToken);

// This is useful for copying a definition, with macros expanded, for e.g. code modification
bool CreateDefinitionCopyMacroExpanded(const ObjectDefinition& definition,
                                       std::vector<Token>& tokensOut);

// Similar to Lisp's gensym, make a globally unique symbol for e.g. macro variables. Use prefix so
// it is still documented as to what it represents. Make sure your generated tokenToChange is
// allocated such that it won't go away until environmentDestroyInvalidateTokens() is called (i.e.
// NOT stack allocated)
// This isn't stable - if a different cakelisp command is run, that could result in different order
// of unique name acquisition
void MakeUniqueSymbolName(EvaluatorEnvironment& environment, const char* prefix,
                          Token* tokenToChange);
// This should be stable as long as the context is managed properly. Code modification may make it
// unstable unless they reset the context on reevaluate, etc.
void MakeContextUniqueSymbolName(EvaluatorEnvironment& environment, const EvaluatorContext& context,
                                 const char* prefix, Token* tokenToChange);

void PushBackTokenExpression(std::vector<Token>& output, const Token* startToken);

void addModifierToStringOutput(StringOutput& operation, StringOutputModifierFlags flag);

void addStringOutput(std::vector<StringOutput>& output, const std::string& symbol,
                     StringOutputModifierFlags modifiers, const Token* startToken);
void addLangTokenOutput(std::vector<StringOutput>& output, StringOutputModifierFlags modifiers,
                        const Token* startToken);
// Splice marker must be pushed to both source and header to preserve ordering in case spliceOutput
// has both source and header outputs
void addSpliceOutput(GeneratorOutput& output, GeneratorOutput* spliceOutput,
                     const Token* startToken);

struct FunctionArgumentTokens
{
	int startTypeIndex;
	int nameIndex;
};
bool parseFunctionSignature(const std::vector<Token>& tokens, int argsIndex,
                            std::vector<FunctionArgumentTokens>& arguments, int& returnTypeStart);
// startInvocationIndex is used for blaming on implicit return type
bool outputFunctionReturnType(const std::vector<Token>& tokens, GeneratorOutput& output,
                              int returnTypeStart, int startInvocationIndex, int endArgsIndex,
                              bool outputSource, bool outputHeader);
bool outputFunctionArguments(const std::vector<Token>& tokens, GeneratorOutput& output,
                             const std::vector<FunctionArgumentTokens>& arguments,
                             bool outputSource, bool outputHeader);

bool tokenizedCTypeToString_Recursive(const std::vector<Token>& tokens, int startTokenIndex,
                                      bool allowArray, std::vector<StringOutput>& typeOutput,
                                      std::vector<StringOutput>& afterNameOutput);

bool CompileTimeFunctionSignatureMatches(EvaluatorEnvironment& environment, const Token& errorToken,
                                         const char* compileTimeFunctionName,
                                         const std::vector<Token>& expectedSignature);

// An interface for building simple generators
struct CStatementOperation
{
	CStatementOperationType type;
	const char* keywordOrSymbol;
	// 0 = operation name
	// 1 = first argument to operation (etc.)
	int argumentIndex;
};

bool CStatementOutput(EvaluatorEnvironment& environment, const EvaluatorContext& context,
                      const std::vector<Token>& tokens, int startTokenIndex,
                      const CStatementOperation* operation, int numOperations,
                      GeneratorOutput& output);

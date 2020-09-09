#include <stdio.h>
#include <string.h>

#include <vector>

#include "Converters.hpp"
#include "Evaluator.hpp"
#include "Generators.hpp"
#include "OutputPreambles.hpp"
#include "RunProcess.hpp"
#include "Tokenizer.hpp"
#include "Utilities.hpp"
#include "Writer.hpp"

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("Need to provide a file to parse\n");
		return 1;
	}

	printf("\nTokenization:\n");

	FILE* file = nullptr;
	const char* filename = argv[1];
	file = fopen(filename, "r");
	if (!file)
	{
		printf("Error: Could not open %s\n", filename);
		return 1;
	}
	else
	{
		printf("Opened %s\n", filename);
	}

	bool verbose = false;

	char lineBuffer[2048] = {0};
	int lineNumber = 1;
	// We need to be very careful about when we delete this so as to not invalidate pointers
	// It is immutable to also disallow any pointer invalidation if we were to resize it
	const std::vector<Token>* tokens = nullptr;
	{
		std::vector<Token>* tokens_CREATIONONLY = new std::vector<Token>;
		while (fgets(lineBuffer, sizeof(lineBuffer), file))
		{
			if (verbose)
				printf("%s", lineBuffer);

			const char* error =
			    tokenizeLine(lineBuffer, filename, lineNumber, *tokens_CREATIONONLY);
			if (error != nullptr)
			{
				printf("%s:%d: error: %s\n", filename, lineNumber, error);
				return 1;
			}

			lineNumber++;
		}

		// Make it const to avoid pointer invalidation due to resize
		tokens = tokens_CREATIONONLY;
	}

	printf("Tokenized %d lines\n", lineNumber - 1);

	if (!validateParentheses(*tokens))
	{
		delete tokens;
		return 1;
	}

	bool printTokenizerOutput = false;
	if (printTokenizerOutput)
	{
		printf("\nResult:\n");

		// No need to validate, we already know it's safe
		int nestingDepth = 0;
		for (const Token& token : *tokens)
		{
			printIndentToDepth(nestingDepth);

			printf("%s", tokenTypeToString(token.type));

			bool printRanges = true;
			if (printRanges)
			{
				printf("\t\tline %d, from line character %d to %d\n", token.lineNumber,
				       token.columnStart, token.columnEnd);
			}

			if (token.type == TokenType_OpenParen)
			{
				++nestingDepth;
			}
			else if (token.type == TokenType_CloseParen)
			{
				--nestingDepth;
			}

			if (!token.contents.empty())
			{
				printIndentToDepth(nestingDepth);
				printf("\t%s\n", token.contents.c_str());
			}
		}
	}

	fclose(file);

	printf("\nParsing and code generation:\n");

	EvaluatorEnvironment environment = {};
	importFundamentalGenerators(environment);
	// Create module definition for top-level references to attach to
	Token modulePsuedoInvocationName = {TokenType_Symbol, "<module>", filename, 1, 0, 1};
	{
		ObjectDefinition moduleDefinition = {};
		moduleDefinition.name = &modulePsuedoInvocationName;
		moduleDefinition.type = ObjectType_Function;
		moduleDefinition.isRequired = true;
		// Will be cleaned up when the environment is destroyed
		GeneratorOutput* compTimeOutput = new GeneratorOutput;
		moduleDefinition.output = compTimeOutput;
		addObjectDefinition(environment, moduleDefinition);
	}
	// TODO Remove test macro
	environment.macros["square"] = SquareMacro;
	EvaluatorContext moduleContext = {};
	moduleContext.scope = EvaluatorScope_Module;
	moduleContext.definitionName = &modulePsuedoInvocationName;
	// Module always requires all its functions
	// TODO: Local functions can be left out if not referenced (in fact, they may warn in C if not)
	moduleContext.isRequired = true;
	GeneratorOutput generatedOutput;
	StringOutput bodyDelimiterTemplate = {};
	bodyDelimiterTemplate.modifiers = StringOutMod_NewlineAfter;
	int numErrors = EvaluateGenerateAll_Recursive(environment, moduleContext, *tokens,
	                                              /*startTokenIndex=*/0, bodyDelimiterTemplate,
	                                              generatedOutput);
	if (numErrors)
	{
		environmentDestroyInvalidateTokens(environment);
		delete tokens;
		return 1;
	}

	if (!EvaluateResolveReferences(environment))
	{
		environmentDestroyInvalidateTokens(environment);
		delete tokens;
		return 1;
	}

	// Final output
	{
		NameStyleSettings nameSettings;
		WriterFormatSettings formatSettings;
		WriterOutputSettings outputSettings;
		outputSettings.sourceCakelispFilename = filename;

		char sourceHeadingBuffer[1024] = {0};
		// TODO: hpp to h support
		// TODO: Strip path from filename
		PrintfBuffer(sourceHeadingBuffer, "#include \"%s.hpp\"\n%s", filename,
		             generatedSourceHeading ? generatedSourceHeading : "");
		outputSettings.sourceHeading = sourceHeadingBuffer;
		outputSettings.sourceFooter = generatedSourceFooter;
		outputSettings.headerHeading = generatedHeaderHeading;
		outputSettings.headerFooter = generatedHeaderFooter;

		printf("\nResult:\n");

		if (!writeGeneratorOutput(generatedOutput, nameSettings, formatSettings, outputSettings))
		{
			environmentDestroyInvalidateTokens(environment);
			delete tokens;
			return 1;
		}
	}

	environmentDestroyInvalidateTokens(environment);
	delete tokens;
	return 0;
}

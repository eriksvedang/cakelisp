#pragma once

#include <vector>
#include <unordered_map>

#include "ModuleManagerEnums.hpp"

#include "Evaluator.hpp"
#include "RunProcess.hpp"
#include "Tokenizer.hpp"

struct ModuleDependency
{
	ModuleDependencyType type;
	std::string name;
};

// Always update both of these. Signature helps validate call
extern const char* g_modulePreBuildHookSignature;
typedef bool (*ModulePreBuildHook)(ModuleManager& manager, Module* module);

// A module is typically associated with a single file. Keywords like local mean in-module only
struct Module
{
	const char* filename;
	const std::vector<Token>* tokens;
	GeneratorOutput* generatedOutput;
	std::string sourceOutputName;
	std::string headerOutputName;

	// Build system
	std::vector<ModuleDependency> dependencies;
	std::vector<std::string> cSearchDirectories;
	std::vector<std::string> additionalBuildOptions;
	// Do not build or link this module. Useful both for compile-time only files (which error
	// because they are empty files) and for files only evaluated for their declarations (e.g. if
	// the definitions are going to be provided via dynamic linking)
	bool skipBuild;

	// These make sense to overload if you want a compile-time dependency
	ProcessCommand compileTimeBuildCommand;
	ProcessCommand compileTimeLinkCommand;

	ProcessCommand buildTimeBuildCommand;
	ProcessCommand buildTimeLinkCommand;

	std::vector<ModulePreBuildHook> preBuildHooks;
};

typedef std::unordered_map<std::string, uint32_t> ArtifactCrcTable;
typedef std::pair<const std::string, uint32_t> ArtifactCrcTablePair;

struct ModuleManager
{
	// Shared environment across all modules
	EvaluatorEnvironment environment;
	Token globalPseudoInvocationName;
	// Pointer only so things cannot move around
	std::vector<Module*> modules;

	// Cached directory, not necessarily the final artifacts directory (e.g. executable-output
	// option sets different location for the final executable)
	std::string buildOutputDir;

	// If an existing cached build was run, check the current build's commands against the previous
	// commands via CRC comparison. This ensures changing commands will cause rebuilds
	ArtifactCrcTable cachedCommandCrcs;
	// If any artifact no longer matches its crc in cachedCommandCrcs, the change will appear here
	ArtifactCrcTable newCommandCrcs;
};

void moduleManagerInitialize(ModuleManager& manager);
void moduleManagerDestroy(ModuleManager& manager);

bool moduleLoadTokenizeValidate(const char* filename, const std::vector<Token>** tokensOut);
bool moduleManagerAddEvaluateFile(ModuleManager& manager, const char* filename, Module** moduleOut);
bool moduleManagerEvaluateResolveReferences(ModuleManager& manager);
bool moduleManagerWriteGeneratedOutput(ModuleManager& manager);
bool moduleManagerBuild(ModuleManager& manager, std::vector<std::string>& builtOutputs);

// Initializes a normal environment and outputs all generators available to it
void listBuiltInGenerators();

#include "version.h"
#include "cxxopts.hpp"

#include <cstdio>

#include "xtal/xtal.h"
#include "xtal/xtal_macro.h"
#include "xtal/xtal_serializer.h"
#include "xtal/xtal_lib/xtal_cstdiostream.h"
#include "xtal/xtal_lib/xtal_winthread.h"
#include "xtal/xtal_lib/xtal_winfilesystem.h"
#include "xtal/xtal_lib/xtal_chcode.h"
#include "xtal/xtal_lib/xtal_errormessage.h"

inline static void inspectCode (xtal::CodePtr code, const std::string& logFilename)
{
	xtal::StringPtr s = code->inspect();

	FILE *logFile = fopen(logFilename.c_str(), "w");

	if (!logFile) {
		fprintf(stderr, "Could not open \"%s\" for writing!\n", logFilename.c_str());
		exit(1);
	}

	printf("Writing inspection to %s...\n", logFilename.c_str());
	printf("  bytecode size:    0x%04X\n", code->bytecode_size());
	printf("  identifiers size: 0x%04X\n", code->identifier_size());

	fprintf(logFile, "%s", s->c_str());
	fclose(logFile);
}

void inspectSerializedFile (const std::string& inputFilename, const std::string& outputFilename)
{
	xtal::FileStreamPtr fs = xtal::xnew<xtal::FileStream>(inputFilename.c_str(), "r");
	xtal::CodePtr code = xtal::ptr_cast<xtal::Code>(fs->deserialize());

	// TODO: check for deserialize errors.

	inspectCode(code, outputFilename);
}

void inspectSourceFile (const std::string& inputFilename, const std::string& outputFilename)
{
	xtal::CodePtr code = xtal::compile_file(inputFilename.c_str());

	if (code) {
		inspectCode(code, outputFilename);
	}

	XTAL_CATCH_EXCEPT(e) {
		fprintf(stderr, "%s\n", e->message()->c_str());
	}
}

void compileFile (const std::string& inputFilename, const std::string& outputFilename)
{
	xtal::CodePtr code = xtal::compile_file(inputFilename.c_str());

	if (code) {
		xtal::FileStreamPtr fs = xtal::xnew<xtal::FileStream>(outputFilename.c_str(), "w");
		fs->serialize(code);
	}

	XTAL_CATCH_EXCEPT(e) {
		fprintf(stderr, "%s\n", e->message()->c_str());
	}
}

int main (int argc, char **argv)
{
	xtal::CStdioStdStreamLib stdStream;
	xtal::WinThreadLib threadLib;
	xtal::WinFilesystemLib fsLib;
	xtal::UTF8ChCodeLib chCodeLib;

	xtal::Setting setting;
	setting.std_stream_lib = &stdStream;
	setting.thread_lib = &threadLib;
	setting.filesystem_lib = &fsLib;
	setting.ch_code_lib = &chCodeLib;

	xtal::initialize(setting);
	xtal::bind_error_message();

	//inspectSerializedFile("C:/s4explore/extract/data/ui/script/lib/common.xtal", "log.txt");
	//inspectSerializedFile("C:/s4explore/extract/data/ui/script/app/world_smash/world_smash_view_select_mii.xtal", "log.txt");
	//inspectSerializedFile("C:/s4explore/extract/data/script/xscene_result.xtal", "log.txt");

	cxxopts::Options options("xtalc", " input");


	try {
		options.add_options()
			("h,help", "Show this message")
			("v,version", "Print version and exit")
			("d,dump", "Dump and inspect compiled XTAL script")
			("p,parse", "Parse and inspect XTAL script")
			("i,interactive", "Starts an assembler REPL")
			("o,output", "Output file", cxxopts::value<std::string>())
			("no-debug", "Don't compile in debug mode")
		;

		options.add_options("hidden")
			("input", "Input file", cxxopts::value<std::string>())
		;

		options.parse_positional("input");
		options.parse(argc, argv);

		if (options.count("version")) {
			printf("xtalc %d.%d.%d%s\n",
				XTALC_VERSION_MAJOR,
				XTALC_VERSION_MINOR,
				XTALC_VERSION_PATCH,
				XTALC_VERSION_FLAG
			);

			return 0;
		}

		if (!options.count("no-debug")) {
			xtal::debug::enable_debug_compile();
		}

		if (options.count("interactive")) {
			char buf[1024];

			while (true) {
				printf("> ");
				gets_s(buf);

				xtal::StringPtr line = xtal::XNew<xtal::String>(buf, strlen(buf));

				xtal::CodePtr code = xtal::eval_compile(line);

				if (code) {
					xtal::StringPtr s = code->inspect();

					printf("%s\n", s->c_str());
				}

				XTAL_CATCH_EXCEPT(e) {
					fprintf(stdout, "%s\n", e->to_s()->c_str());
				}
			}

			return 0;
		}

		if (options.count("help") || !options.count("input")) {
			printf("%s\n", options.help({""}).c_str());
			return 0;
		}

		std::string input = options["input"].as<std::string>();
		std::string output;

		if (options.count("output")) {
			output = options["output"].as<std::string>();
		}

		// FIXME: i don't like how this feels.
		if (options.count("dump")) {
			inspectSerializedFile(input, options.count("output") ? output : "log.txt");
		} else if (options.count("parse")) {
			inspectSourceFile(input,  options.count("output") ? output : "log.txt");
		} else {
			compileFile(input,  options.count("output") ? output : "out.xtal");
		}

	} catch (const cxxopts::OptionException& e) {
		// TODO: catch individual exceptions for
		// better (read: existent) error messages.
		fprintf(stderr, "%s\n", options.help({""}).c_str());

		return 1;
	}

	xtal::uninitialize();

	return 0;
}

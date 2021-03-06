#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "Utils/Logger.h"
#include "Utils/Debug.h"
#include "Utils/Finally.h"

#include "Funal/Delegate.h"

#include "IO/FormattedWriter.h"
#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {

typedef Delegate<void(IO::FormattedWriter&)> TestFunction;

class TestGroup
{
	TestGroup* mParentTestGroup;
public:
	bool Yes;
	ILogger* Logger;
	IO::FormattedWriter& Output;
	StringView Category;
	Utils::SourceInfo ErrorInfo;

	operator bool() const {return Yes;}
	TestGroup(ILogger* logger, IO::FormattedWriter& output, StringView category);
	
	TestGroup(ILogger* logger, IO::FormattedWriter& output,
		StringView category, const TestFunction& funcToTest):
		TestGroup(logger, output, category)
	{
		if(!Yes) return;
		auto oldCallback = Utils::gFatalErrorCallback;
		Utils::gFatalErrorCallback = internalErrorTestFail;
		tryCallTest(funcToTest);
		Utils::gFatalErrorCallback = oldCallback;
	}

	TestGroup(StringView category);
	TestGroup(StringView category, const TestFunction& funcToTest);
	~TestGroup();

	static TestGroup* GetCurrent() {return currentTestGroup;}
	void PrintUnitTestResult();

	static int GetTotalTests() {return totalTestsPassed + totalTestsFailed;}
	static int GetTotalTestsPassed() {return totalTestsPassed;}
	static int GetTotalTestsFailed() {return totalTestsFailed;}


	static int YesForNestingLevel;
private:
	static TestGroup* currentTestGroup;
	static int nestingLevel;
	bool mHadChildren;
	int mFailedChildren, mPassedChildren;
	static int totalTestsFailed, totalTestsPassed;
	TestGroup& operator=(const TestGroup&) = delete;

	void consoleAskToEnableTest();

	struct TestException {};
	void tryCallTest(const TestFunction& funcToTest)
	{
	#ifdef INTRA_EXCEPTIONS_ENABLED
		try { //���������� ��� ������ ����������� ������ �� ���������� ����� ������ ������� ����� ����������
	#endif
			funcToTest(Output);
	#ifdef INTRA_EXCEPTIONS_ENABLED
	#ifdef _MSC_VER
	#pragma warning(disable: 4571)
	#endif
		} catch(TestException) {}
		catch(...) {if(Logger) Logger->Error("Unknown exception caught!", INTRA_SOURCE_INFO);}
	#endif
	}

	static void internalErrorTestFail(const Utils::SourceInfo& srcInfo, StringView msg)
	{
		static bool alreadyCalled = false;
		if(alreadyCalled) abort();
		alreadyCalled = true;
		INTRA_FINALLY(alreadyCalled = false);
		processError(srcInfo, msg);
	#ifdef INTRA_EXCEPTIONS_ENABLED
		if(GetCurrent() != null)
		{
			GetCurrent()->Logger->Error("Test stack unwinding...");
			throw TestException();
		}
	#endif
		exit(totalTestsFailed + 1);
	}

	static void processError(const Utils::SourceInfo& srcInfo, StringView msg);
};

}

INTRA_WARNING_POP

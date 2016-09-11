﻿#include "IO/Stream.h"
#include "IO/File.h"
#include "Containers/String.h"
using namespace Intra;
using namespace Intra::IO;

static String MakeIdentifierFromPath(StringView path, StringView prefix=null)
{
	String result = DiskFile::ExtractName(path);
	result.AsRange().Transform([](char c){return AsciiSet::LatinAndDigits.Contains(c)? c: '_';});
	result = prefix+result;
	if(byte(result[0]-'0')<=9) result = '_'+result;
	return result;
}

StringView PathFromCmdLine(const char* path)
{
	return StringView(path).Trim('"').Trim(Range::IsSpace<char>);
}

static void ParseCommandLine(int argc, const char* argv[], String& inputFilePath, String& outputFilePath,
	String& binArrName, int& valuesPerLine, bool& notabs, bool& nospaces, bool& singleline, bool& endline)
{
	inputFilePath = null;
	outputFilePath = null;
	binArrName = null;
	valuesPerLine = 0;
	notabs = false;
	nospaces = false;
	singleline = false;
	endline = false;

	for(int i=1; i<argc; i++)
	{
		if(argv[i][0]!='-')
		{
			inputFilePath = PathFromCmdLine(argv[i]);
			continue;
		}

		if(StringView(argv[i])=="-notabs")
		{
			notabs=true;
			continue;
		}
		if(StringView(argv[i])=="-nospaces")
		{
			nospaces=true;
			continue;
		}
		if(StringView(argv[i])=="-singleline")
		{
			singleline=true;
			continue;
		}
		if(StringView(argv[i])=="-endline")
		{
			endline=true;
			continue;
		}

		if(StringView(argv[i])=="-o")
		{
			outputFilePath = PathFromCmdLine(argv[++i]);
			continue;
		}
		if(StringView(argv[i])=="-varname")
		{
			binArrName = PathFromCmdLine(argv[++i]);
			continue;
		}
		if(StringView(argv[i])=="-per-line")
		{
			valuesPerLine = StringView(argv[++i]).ParseAdvance<int>();
			continue;
		}
	}

	if(outputFilePath==null) outputFilePath = inputFilePath+".c";
	if(binArrName==null) binArrName = MakeIdentifierFromPath(inputFilePath);
	if(valuesPerLine==0) valuesPerLine = 32;
}



inline int ByteToStr(byte x, char* dst)
{
	int len=0;
	if(x>=100) dst[len++] = '0'+x/100;
	if(x>=10) dst[len++] = '0'+x/10%10;
	dst[len++] = '0'+x%10;
	return len;
}


void ConvertFile(StringView inputFilePath, StringView outputFilePath, StringView binArrName,
	int valuesPerLine, bool notabs, bool nospaces, bool singleline, bool endline)
{
	DiskFile::Reader input(inputFilePath);
	if(input==null)
	{
		Console.PrintLine("File ", inputFilePath, " is not opened!");
		return;
	}
	ArrayRange<const byte> src = input.Map<byte>();
	const byte* srcBytes = src.Begin;
	DiskFile::Writer dst(outputFilePath);
	dst.Print("const unsigned char ", binArrName, "[] = {");
	if(!singleline)
	{
		dst.PrintLine();
		if(!notabs) dst.Print('\t');
	}
	char dataToPrint[10]={',', ' '};
	static const char dataToPrintPerLine[4]="\r\n\t";
	const int sizeToPrintPerLineCount = 3-int(notabs);
	int sizeToPrint = 2-int(nospaces);

	if(src!=null) dst << (int)srcBytes[0];
	if(singleline) for(size_t i=1; i<src.Count(); i++)
	{
		int n = sizeToPrint;
		n += ByteToStr(srcBytes[i], dataToPrint+n);
		dst.WriteData(dataToPrint, n);
	}
	else for(size_t i=1; i<src.Count(); i++)
	{
		int n = sizeToPrint;
		if(!singleline && i%valuesPerLine==0)
		{
			core::memcpy(dataToPrint+n, dataToPrintPerLine, sizeToPrintPerLineCount);
			n += sizeToPrintPerLineCount;
		}
		n += ByteToStr(srcBytes[i], dataToPrint+n);
		dst.WriteData(dataToPrint, n);
	}
	if(!singleline) dst.PrintLine();
	dst.Print("};");
	if(endline) dst.PrintLine();
}



int INTRA_CRTDECL main(int argc, const char* argv[])
{
	if(argc<2)
	{
		Console.PrintLine("Usage:");
		Console.PrintLine(argv[0], " <inputFilePath> [-o <outputFilePath.c>] [-varname <binaryArrayName>] [-per-line <32>] [-notabs] [-nospaces] [-singleline] [-endline]");
		return 0;
	}

	String inputFilePath, outputFilePath, binArrName;
	int valuesPerLine;
	bool notabs, nospaces, singleline, endline;
	ParseCommandLine(argc, argv, inputFilePath, outputFilePath, binArrName, valuesPerLine, notabs, nospaces, singleline, endline);
	ConvertFile(inputFilePath, outputFilePath, binArrName, valuesPerLine, notabs, nospaces, singleline, endline);

	return 0;
}
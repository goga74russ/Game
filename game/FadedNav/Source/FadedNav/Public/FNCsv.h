#pragma once

#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Design tables live in docs/systems/** as CSV (Excel, Russian locale: UTF-8 BOM, ';'). The game reads them at startup,
// so the table IS the tuning. Dev path: <repo>/docs/...; packaged build: <project>/Data/... (staged copy).
struct FFNCsv
{
	TArray<FString> Head;
	TArray<TArray<FString>> Rows;
	FString Path;

	int32 Col(const TCHAR* Name) const { return Head.IndexOfByKey(FString(Name)); }
	FString Str(const TArray<FString>& Row, int32 C) const { return Row.IsValidIndex(C) ? Row[C].TrimStartAndEnd() : FString(); }
	bool Has(const TArray<FString>& Row, int32 C) const { return !Str(Row, C).IsEmpty(); }
	float Num(const TArray<FString>& Row, int32 C, float Default = 0.f) const
	{
		const FString S = Str(Row, C);
		return S.IsEmpty() ? Default : FCString::Atof(*S.Replace(TEXT(","), TEXT("."))); // "0,5" from a Russian Excel
	}

	static TArray<FString> Split(const FString& Line, TCHAR Delim)
	{
		TArray<FString> Out;
		FString Cur;
		bool bQuoted = false;
		for (int32 i = 0; i < Line.Len(); ++i)
		{
			const TCHAR C = Line[i];
			if (C == '"') { if (bQuoted && i + 1 < Line.Len() && Line[i + 1] == '"') { Cur.AppendChar('"'); ++i; } else { bQuoted = !bQuoted; } }
			else if (C == Delim && !bQuoted) { Out.Add(Cur); Cur.Reset(); }
			else { Cur.AppendChar(C); }
		}
		Out.Add(Cur);
		return Out;
	}

	// RelPath under docs/systems, e.g. "skills/C1-Yav/skills.csv". Returns false if no file was found.
	bool Load(const FString& RelPath)
	{
		const FString Candidates[] = {
			FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../docs/systems") / RelPath),
			FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Data") / RelPath),
		};
		for (const FString& P : Candidates)
		{
			TArray<FString> Lines;
			if (!FFileHelper::LoadFileToStringArray(Lines, *P) || Lines.Num() < 1) { continue; }
			const TCHAR Delim = Lines[0].Contains(TEXT(";")) ? TEXT(';') : TEXT(',');
			Head = Split(Lines[0], Delim);
			for (FString& H : Head) { H.TrimStartAndEndInline(); }
			Rows.Reset();
			for (int32 L = 1; L < Lines.Num(); ++L)
			{
				if (!Lines[L].TrimStartAndEnd().IsEmpty()) { Rows.Add(Split(Lines[L], Delim)); }
			}
			Path = P;
			return true;
		}
		return false;
	}
};

﻿#include "JavascriptOutputDevice.h"
#include "UObject/UObjectThreadContext.h"

/** This class is to capture all log output even if the log window is closed */
class FJavascriptOutputDevice : public FOutputDevice
{
public:
	UJavascriptOutputDevice* OutputDevice;

	FJavascriptOutputDevice(UJavascriptOutputDevice* InOutputDevice)
	{
		OutputDevice = InOutputDevice;

		GLog->AddOutputDevice(this);
		GLog->SerializeBacklog(this);
	}

	~FJavascriptOutputDevice()
	{
		// At shutdown, GLog may already be null
		if (GLog != nullptr)
		{
			GLog->RemoveOutputDevice(this);
		}
	}
	
protected:

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
	{		
		static bool bIsReentrant = false;
		if (bIsReentrant) return;

		TGuardValue<bool> ReentrantGuard(bIsReentrant, true);
		if (!OutputDevice->IsUnreachable() && !FUObjectThreadContext::Get().IsRoutingPostLoad)
		{
			OutputDevice->OnMessage(V, (ELogVerbosity_JS)Verbosity, Category);
		}		
	}
};

UJavascriptOutputDevice::UJavascriptOutputDevice(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OutputDevice = MakeShareable(new FJavascriptOutputDevice(this));
	}	
}

void UJavascriptOutputDevice::BeginDestroy()
{
	Super::BeginDestroy();

	OutputDevice.Reset();
}

void UJavascriptOutputDevice::Kill()
{
	OutputDevice.Reset();	
}

void UJavascriptOutputDevice::Log(FName Category, ELogVerbosity_JS _Verbosity, const FString& Filename, int32 LineNumber, const FString& Message)
{
#if NO_LOGGING
	
#else
	auto Verbosity = (ELogVerbosity::Type)_Verbosity;	
	FMsg::Logf_Internal(TCHAR_TO_ANSI(*Filename), LineNumber, Category, Verbosity, TEXT("%s"), *Message);
	if (Verbosity == ELogVerbosity::Fatal)
	{
#ifdef UE_DEBUG_BREAK_AND_PROMPT_FOR_REMOTE
		UE_DEBUG_BREAK_AND_PROMPT_FOR_REMOTE();
#else
		_DebugBreakAndPromptForRemote();
#endif
		FDebug::AssertFailed("", TCHAR_TO_ANSI(*Filename), LineNumber, TEXT("%s"), *Message);
	}
#endif
}

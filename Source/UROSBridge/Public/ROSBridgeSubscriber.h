// Copyright 2017, Institute for Artificial Intelligence - University of Bremen

#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "ROSBridgeMsg.h"

class UROSBRIDGE_API FROSBridgeSubscriber 
{
    FString Type;
    FString Topic;

public:

    FROSBridgeSubscriber(FString InType, FString InTopic):
        Type(InType), Topic(InTopic)
    {
    }

    virtual ~FROSBridgeSubscriber() 
	{
    }

    virtual FString GetMessageType() const 
	{
        return Type;
    }

    virtual FString GetMessageTopic() const 
	{
        return Topic;
    }

    virtual TSharedPtr<FROSBridgeMsg> ParseMessage(TSharedPtr<FJsonObject> JsonObject) const = 0;

    virtual void Callback(TSharedPtr<FROSBridgeMsg> Msg) = 0;
};

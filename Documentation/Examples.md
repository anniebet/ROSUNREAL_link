### UROSBridge

#### Connection and disconnection

UROSBridge could be used in Unreal Actors or in timers. To use it in actors, we need to add a smart pointer to ROSBridgeHandler first: 

```
UCLASS()
class PROJECT_API AROSActor : public AActor
{
    GENERATED_BODY()

public:
    TSharedPtr<FROSBridgeHandler> Handler; 
    ...
}
```

In Actor's `BeginPlay()` function, create handler instance and connect to the websocket server:

```
void AROSActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Set websocket server address to ws://127.0.0.1:9001
    Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT("127.0.0.1"), 9001));
    
    // Add topic subscribers and publishers
    // Add service clients and servers
    
    // Connect to ROSBridge Websocket server.
    Handler->Connect();
}
```

In Actor's `Tick(float)` function, add `Handler->Render()` function to let handler process incoming messages in the message queue. 

```
void AROSActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Do something
    
    Handler->Render();
}
```

In Actor's `Logout` or `EndPlay` function, add `Handler->Disconnect()` function before the parent class ends. 

```
void AROSActor::EndPlay(const EEndPlayReason::Type Reason)
{
    Handler->Disconnect(); 
    // Disconnect the handler before parent ends

    Super::EndPlay(Reason);
}
```

#### Publish Message 

To publish message to a topic, we need to first advertise the topic in ROS bridge. In AROSActor's class definition, Add a ROSBridgePublisher smart pointer. 

```
UCLASS()
class PROJECT_API AROSActor : public AActor
{
    GENERATED_BODY()

public:
    TSharedPtr<FROSBridgeHandler> Handler; 
    TSharedPtr<FROSBridgePublisher> Publisher; // This
    ...
}
```

In AROSActor's `BeginPlay()` function, create the publisher using its **type** and **topic**, and then register it to the ROSBridgeHandler. 

```
void AROSActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Set websocket server address to ws://127.0.0.1:9001
    Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT("127.0.0.1"), 9001));
    
    // **** Create publishers here ****
    Publisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("sensor_msgs/JointState"), TEXT("/talker")));
    Handler->AddPublisher(Publisher); 
    
    // Connect to ROSBridge Websocket server.
    Handler->Connect();
}
```

#### Subscribe to Topics

This plugin uses `FROSBridgeSubscriber` class interface to subscribe to topics. We need to extend a `FROSBridgeSubscriber` subclass for each topic we would like to subscribe to, implementing the constructor, destructor, `ParseMessage` function and `CallBack` function. 

##### Include Messages

In class header, the UROSBridge Message class header should be included. 

```
#include "ROSBridgeSubscriber.h"
#include "std_msgs/String.h"
#include "Core.h"
class FROSStringSubScriber : public FROSBridgeSubscriber {
   FROSStringSubScriber(FString Topic_); 
   ~FROSStringSubScriber() override; 
   TSharedPtr<FROSBridgeMsg> ParseMessage(TSharedPtr<FJsonObject> JsonObject) const override; 
   void CallBack(TSharedPtr<FROSBridgeMsg> msg) const override; 
}
```

##### Constructor 

In class constructor, we need to call the parent class constructor to set type and topic for this subscriber. 

```
FROSStringSubScriber::FROSStringSubScriber(FString Topic_):
    FROSBridgeSubscriber(TEXT("std_msgs/String"), Topic_) {}
``` 

##### Destructor 

Class destructors will be required if you need to do some cleaning work after the ROS bridge client disconnects. 

```
FROSStringSubScriber::~FROSStringSubScriber() {};
```

##### ParseMessage

`ParseMessage` function is used by ROSBridgeHandler to convert a `JSONObject` to `FROSBridgeMsg` instance. Create a ROSBridgeMessage class with specified message type (e.g. `FROSBridgeMsgStdmsgsString`) and call its `FromJson` method to parse the JSON message. Finally convert the pointer to a `FROSBridgeMsg` pointer. 

```
TSharedPtr<FROSBridgeMsg> FROSStringSubScriber::ParseMessage
(TSharedPtr<FJsonObject> JsonObject) const
{
    TSharedPtr<FROSBridgeMsgStdmsgsString> StringMessage =
        MakeShareable<FROSBridgeMsgStdmsgsString>(new FROSBridgeMsgStdmsgsString());
    StringMessage->FromJson(JsonObject);
    return StaticCastSharedPtr<FROSBridgeMsg>(StringMessage);
}
```

##### CallBack

`CallBack` is the callback function called when a new message comes and is successfully parsed to a `ROSBridgeMsg` instance. In this function, we need to first down-cast the `FROSBridgeMsg` pointer to a pointer of its subclass. 

```
void CallBack(TSharedPtr<FROSBridgeMsg> msg) const 
{
    TSharedPtr<FROSBridgeMsgStdmsgsString> StringMessage = StaticCastSharedPtr<FROSBridgeMsgStdmsgsString>(msg);
    // downcast to subclass using StaticCastSharedPtr function
    
    UE_LOG(LogTemp, Log, TEXT("Message received! Content: %s"), *StringMessage->GetData());
    // do something with the message

    return;
}
```

##### In Unreal Actor

In Unreal Actors, before the ROS Bridge Handler connects to the server, we need to add pointer to subscriber to the subscriber list first. 

```
void AROSActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Set websocket server address to ws://127.0.0.1:9001
    Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT("127.0.0.1"), 9001));
    
    // Add topic subscribers and publishers
    TSharedPtr<FROSStringSubScriber> Subscriber = 
        MakeShareable<FROSStringSubScriber>(new FROSStringSubScriber(TEXT("/chatter")));
    Handler->AddSubscriber(Subscriber);
    
    // Connect to ROSBridge Websocket server.
    Handler->Connect();
}
```

When the ROS Bridge Handler disconnects to server, it automatically destroys all subscriber instances. 

#### Request Service

A service consists of two parts: Request and Response. Clients send out requests, and then get response from server. Servers process received requests and send out response. 

To send service requests in UROSBridge, we need to create a service client class first. This class should extend the FROSBridgeSrvClient and implement the constructor and a callback function. Below is an example of service "AddTwoInts" client.  

```
#pragma once
#include "ROSBridgeSrvClient.h"
#include "tutorial_srvs/AddTwoInts.h"
class FROSAddTwoIntsClient final : public FROSBridgeSrvClient
{
public:
    FROSAddTwoIntsClient(FString Name):
        FROSBridgeSrvClient(Name, TEXT("beginner_tutorials/AddTwoInts")) {}
    void CallBack(TSharedPtr<FROSBridgeSrv::SrvRequest> Request, TSharedPtr<FROSBridgeSrv::SrvResponse> Response) const override
    {
        TSharedPtr<FROSBridgeSrvRospytutorialsAddTwoInts::Request> Request_ =
            StaticCastSharedPtr<FROSBridgeSrvRospytutorialsAddTwoInts::Request>(Request);
        TSharedPtr<FROSBridgeSrvRospytutorialsAddTwoInts::Response> Response_=
            StaticCastSharedPtr<FROSBridgeSrvRospytutorialsAddTwoInts::Response>(Response);
        // Do downcast to convert Request and Response to corresponding types
        //        
        UE_LOG(LogTemp, Log, TEXT("Add Two Ints: %d + %d = %d"), Request_->GetA(), Request_->GetB(), Response_->GetSum());
    }
};
```

Then in ROSActor, we can send service requests using the following function: 

```
TSharedPtr<FROSAddTwoIntsClient> ServiceClient = 
    MakeShareable<FROSAddTwoIntsClient>(new FROSAddTwoIntsClient(TEXT("add_two_ints")));

int NumA = FMath::RandRange(1, 10000);
int NumB = FMath::RandRange(1, 10000);
TSharedPtr<FROSBridgeSrv::SrvRequest> Request = MakeShareable(new FROSBridgeSrvRospytutorialsAddTwoInts::Request(NumA, NumB));
// Create a request instance with request parameters
TSharedPtr<FROSBridgeSrv::SrvResponse> Response = MakeShareable(new FROSBridgeSrvRospytutorialsAddTwoInts::Response());
// Create an empty response instance 
Handler->CallService(ServiceClient, Request, Response);
```

Notice: The `CallService` is not a blocking function, i.e. it will not block the main actor thread to wait for service response but it will call the callback function once it receives the response in following ticks. 

#### Send Response to Service Requests

The plugin can also works as a "server" side who receives ros service requests from and returns response to the clients. 

To process service requests in UROSBridge, we need to create a service server class first. This class should extend the FROSBridgeSrvServer and implement the constructor, `FromJson`, and `CallBack` function. It is very similar to ROS Bridge Subscriber classes but the only difference is that the return type of `CallBack` is `TSharedPtr<FROSBridgeSrv::SrvResponse>` rather than `void`. Below is an example of service AddTwoInts server. 

```
 #pragma once
 
 #include "ROSBridgeSrvServer.h"
 #include "tutorial_srvs/AddTwoInts.h"
 
 class FROSAddTwoIntsServer final : public FROSBridgeSrvServer
 {
 public:
     FROSAddTwoIntsServer(FString Name):
         FROSBridgeSrvServer(Name, TEXT("beginner_tutorials/AddTwoInts")) {}
          
     TSharedPtr<FROSBridgeSrv::SrvRequest> FromJson(TSharedPtr<FJsonObject> JsonObject) const override
     {
         TSharedPtr<FROSBridgeSrvRospytutorialsAddTwoInts::Request> Request_ =
             MakeShareable(new FROSBridgeSrvRospytutorialsAddTwoInts::Request());
         Request_->FromJson(JsonObject);
         return TSharedPtr<FROSBridgeSrv::SrvRequest>(Request_);
     } 
     
     TSharedPtr<FROSBridgeSrv::SrvResponse> CallBack(TSharedPtr<FROSBridgeSrv::SrvRequest> Request) const override
     {
         TSharedPtr<FROSBridgeSrvRospytutorialsAddTwoInts::Request> Request_ =
             StaticCastSharedPtr<FROSBridgeSrvRospytutorialsAddTwoInts::Request>(Request);
 
         int64 Sum = Request_->GetA() + Request_->GetB();
         UE_LOG(LogTemp, Log, TEXT("Service [%s] Server: Add Two Ints: %d + %d = %d"), *Name, Request_->GetA(), Request_->GetB(), Sum);
         return MakeShareable<FROSBridgeSrv::SrvResponse>
                   (new FROSBridgeSrvRospytutorialsAddTwoInts::Response(Sum));
     }

 }
```

In Actor, before connection, first register the server to ROS bridge, then it will process incoming service requests automatically. After disconnection, the server will be automatically destroyed. 

```
void AROSActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Set websocket server address to ws://127.0.0.1:9001
    Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT("127.0.0.1"), 9001));
    
    // Add service clients and servers
    TSharedPtr<FROSAddTwoIntsServer> ServiceServer = MakeShareable<FROSAddTwoIntsServer>(new FROSAddTwoIntsServer(TEXT("add_two_ints_2")));
    Handler->AddServiceServer(ServiceServer);
    
    // Connect to ROSBridge Websocket server.
    Handler->Connect();
}
```

#### Add More Message / Service Types

This plugin already has support for `std_msgs`, `geometry_msgs` and `std_srvs`, but sometimes other types of message / service will be required. We can add new message classes to the plugin or directly to the project source folder.   

##### Message / Topic 

Messages should extend the `FROSBridgeMsg` class, and implement the following functions: 

- Constructor & Destructor
- `void FromJson(TSharedPtr<FJsonObject> JsonObject)`
    - set all the properties from the JsonObject
    - For numbers, use `JsonObject->GetNumberField(FieldName)`
    - For strings, use `JsonObject->GetStringField(FieldName)`
    - For other message types or ros time, use `ClassType::GetFromJson(JsonObject->GetObjectField(FieldName))`    
    - For array, first get JsonValue array using `TArray<TSharedPtr<FJsonValue>> PointsPtrArray = JsonObject->GetArrayField(TEXT("points"))`, then for each element `ptr` in the array, call `ptr->AsObject()`, `ptr->AsNumber()` or `ptr->AsString()` function to get its value. 
    - It is recommended to always do type checking using Unreal `check(...)` macro
- `static CLASS_NAME GetFromJson(TSharedPtr<FJsonObject> JsonObject)`
    - the static version of FromJson()
    - Create a new message class and use `FromJson(JsonObject)` to set its properties
- `virtual FString ToString () const`
    - Create a string with all of the properties of this class for printing. 
- `virtual TSharedPtr<FJsonObject> ToJsonObject() const`
    - Create an FJsonObject and save all of the properties of this message instance to the JsonObject. 
    - For numbers, use `JsonObject->SetNumberField(FieldName, Value)`
    - For strings, use `JsonObject->SetStringField(FieldName, Value)`
    - For other message types or ros time, use `JsonObject->SetObjectField(FieldName, Field.ToJsonObject())`    
    - For arrays, we first create a JsonValue array using `TArray<TSharedPtr<FJsonValue>> PtrArray`, then add shared pointer to new created FJsonValueObject / FJsonValueNumber / FJsonValueString to the array, and finally set the field value to this array using `Object->SetArrayField(FieldName, PtrArray);`
    
There are several good examples to follow when writing message classes. `geometry_msgs/Vector3` is a message class with only built-in types; `geometry_msgs/Accel` is a message class which includes other messages; `geometry_msgs/Polygon` is a message class with arrays.

##### Services

Services consists of two parts, **request** and **response**, each of which is a ROS message class. In the plugin,  for each service we define a class extending to the `FROSBridgeSrv` base class, inside which we define `Request` and `Response` classes respectively extending to the `SrvRequest` and `SrvResponse` class. Like messages, in each class we need to implement `FromJson`, `ToString`, `ToJsonObject` and static `GetFromJson` method functions. 

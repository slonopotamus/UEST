#pragma once

template<typename T>
static TEnableIf<TIsEnumClass<T>::Value, FString>::Type ToString(T Value)
{
	return StaticEnum<T>()->GetNameStringByValue(static_cast<int64>(Value));
}

static FString ToString(FString Value)
{
	return Value;
}

static FString ToString(const float Value)
{
	return FString::Printf(TEXT("%.3f"), Value);
}

static FString ToString(const double Value)
{
	return FString::Printf(TEXT("%.3f"), Value);
}

static FString ToString(const int32 Value)
{
	return FString::Printf(TEXT("%d"), Value);
}

static FString ToString(const int64 Value)
{
	return FString::Printf(TEXT("%lld"), Value);
}

static FString ToString(const uint32 Value)
{
	return FString::Printf(TEXT("%u"), Value);
}

static FString ToString(const uint64 Value)
{
	return FString::Printf(TEXT("%llu"), Value);
}

static FString ToString(const UObject& Value)
{
	return GetPathNameSafe(&Value);
}

static FString ToString(const FVector& Value)
{
	return Value.ToString();
}

static FString ToString(const FRotator& Value)
{
	return Value.ToString();
}

template<typename T>
static FString ToString(const T* Value)
{
	return Value ? ToString(*Value) : TEXT("nullptr");
}

template<typename T>
static FString ToString(const TSharedPtr<T>& Value)
{
	return Value ? ToString(Value.Get()) : TEXT("nullptr");
}

template<typename T>
static FString ToString(const TObjectPtr<T>& Value)
{
	return ToString(Value.Get());
}

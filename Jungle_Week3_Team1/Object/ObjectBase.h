#pragma once

class UObjectBase
{

public:
	virtual UClass* GetClass() const	
	{
		return nullptr;
	}
	static UClass* StaticClass() { return nullptr; }
};
#pragma once

#define DECLARE_OBJECT(ClassName, ParentName)\
    static UClass* GetPrivateStaticClass();\
    public:\
        typedef ParentName Parent;\
        typedef ClassName ThisClass;\
        inline static UClass* StaticClass()\
        {\
	       return GetPrivateStaticClass();\
        }\
        virtual UClass* GetClass() const override\
        {\
           return StaticClass();\
        }


#define IMPLEMENT_OBJECT(ClassName, ParentName)\
UClass* ClassName::GetPrivateStaticClass()\
{\
    static UClass ClassInfo = { \
           #ClassName,\
           sizeof(ClassName),\
           ParentName::StaticClass()\
    }; \
    return &ClassInfo; \
}
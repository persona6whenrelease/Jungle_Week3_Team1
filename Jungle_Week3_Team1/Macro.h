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



#define IMPLEMENT_BASE_CLASS(ClassName)\
UClass* ClassName::GetPrivateStaticClass()\
{\
    static UClass ClassInfo = { \
           #ClassName,\
           sizeof(ClassName),\
           nullptr\
    }; \
    return &ClassInfo; \
}



#define IMPLEMENT_CLASS(ClassName, ParentName)\
UClass* ClassName::GetPrivateStaticClass()\
{\
    static UClass ClassInfo = { \
           #ClassName,\
           sizeof(ClassName),\
           ParentName::StaticClass()\
    }; \
    return &ClassInfo; \
}
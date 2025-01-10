#pragma once

template <typename Member>
class TPrivateAccessor {
public:
	inline static typename Member::Type MemberPtr;
};

template <typename Member, typename Member::Type Ptr>
struct TStaticPtrInit {
	struct FConstructPrivateAccessor {
		FConstructPrivateAccessor() {
			TPrivateAccessor<Member>::MemberPtr = Ptr;
		}
	};
	inline static FConstructPrivateAccessor TriggerConstruct;
};

#define DECLARE_PRIVATE_ACCESS(Typename,MemberName,MemberType)\
struct Typename##MemberName##Access{using Type=MemberType Typename::*;};\
template struct TStaticPtrInit<Typename##MemberName##Access,&##Typename##::##MemberName>;\

#define PRIVATE_ACCESS(Typename,MemberName)\
TPrivateAccessor<Typename##MemberName##Access>::MemberPtr
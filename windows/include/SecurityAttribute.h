#ifndef __SECURITYATTRIBUTE_H__
#define __SECURITYATTRIBUTE_H__


//#include <winnt.h> // for security attributes constants
 // for ACL
 #include "common.h"


class SecurityAttribute {
private:
	PSECURITY_DESCRIPTOR pd;
	SECURITY_ATTRIBUTES sa;
	void _Init();
public:
	SecurityAttribute() : pd(NULL) { _Init(); }
	SECURITY_ATTRIBUTES *get_attr();
};

extern LPSECURITY_ATTRIBUTES lpSA;

#endif
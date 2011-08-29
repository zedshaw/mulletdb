#ifndef _mulletdb_H
#define _mulletdb_H 1
#include <grace/daemon.h>
#include <grace/configdb.h>

//  -------------------------------------------------------------------------
/// Implementation template for application config.
//  -------------------------------------------------------------------------
typedef configdb<class MulletDBApp> appconfig;

//  -------------------------------------------------------------------------
/// Main daemon class.
//  -------------------------------------------------------------------------
class MulletDBApp : public daemon
{
public:
		 				 MulletDBApp (void);
		 				~MulletDBApp (void);
		 	
	int					 main (void);
	
protected:
	bool				 confLog (config::action act, keypath &path,
								  const value &nval, const value &oval);


	appconfig			 conf;
};

enum STATUS_CODES {
    OK=200,
    INVALID_REQUEST=500,
    OPERATION_FAILED=400,
    NOT_FOUND=404
};

#endif

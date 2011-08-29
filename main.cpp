#include "mulletdb.h"
#include "queryhandler.h"


$appobject (MulletDBApp);

// ==========================================================================
// CONSTRUCTOR MulletDBApp
// ==========================================================================
MulletDBApp::MulletDBApp (void)
	: daemon ("com.mulletdb.mulletdb"),
	  conf (this)
{
	opt = $("-h", $("long", "--help"));
}

// ==========================================================================
// DESTRUCTOR MulletDBApp
// ==========================================================================
MulletDBApp::~MulletDBApp (void)
{
}


// ==========================================================================
// METHOD MulletDBApp::main
// ==========================================================================
int MulletDBApp::main (void)
{
	string conferr; ///< Error return from configuration class.
	
	// Add watcher value for event log. System will daemonize after
	// configuration was validated.
	conf.addwatcher("system/eventlog", &MulletDBApp::confLog);
	
	// Load will fail if watchers did not valiate.
	if (! conf.loadini ("etc:mulletdb.conf", conferr))
	{
		ferr.writeln ("%% Error loading configuration: %s" %format (conferr));
		return 1;
	}

    value server_config = conf["server"];

    if(server_config.exists("user")) {
        log::write(log::info, "main", "changing user to %s" %format(server_config["user"]));
        settargetuser(server_config["user"]);
    }

    if(server_config.exists("group")) {
        log::write(log::info, "main", "changing group to %s" %format(server_config["group"]));
        settargetgroups($(server_config["group"]));
    }

    if(server_config.exists("chroot")) {
        log::write(log::info, "main", "changing root to %s" %format(server_config["chroot"]));
        chroot(server_config["chroot"]);
    }

	daemonize ();

	log::write (log::info, "main", "mulletdb started");
	value ev;

    QueryHandler handler(server_config);
    handler.spawn();
	
	while (true)
	{
		ev = waitevent ();
		if (ev.type() == "shutdown") break;
	}

    handler.die();
    sleep(2);

	log::write (log::info, "main", "Shutting down");
	stoplog();
	return 0;
}


// ==========================================================================
// METHOD MulletDBApp::confLog
// ==========================================================================
bool MulletDBApp::confLog (config::action act, keypath &kp,
							  const value &nval, const value &oval)
{
	string tstr;
	
	switch (act)
	{
		case config::isvalid:
			// Check if the path for the event log exists.
			tstr = strutil::makepath (nval.sval());
			if (! tstr.strlen()) return true;
			if (! fs.exists (tstr))
			{
				ferr.writeln ("%% Log path %s does not exist" %format (tstr));
				return false;
			}
			return true;
			
		case config::create:
			// Set the event log target and daemonize.
			fout.writeln ("%% Event log: %s\n" %format (nval));
			addlogtarget (log::file, nval.sval(), log::all, 1024*1024);
			return true;
	}
	
	return false;
}

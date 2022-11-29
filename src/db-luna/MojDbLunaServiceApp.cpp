// Copyright (c) 2009-2021 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0


#include "db-luna/MojDbLunaServiceApp.h"
#include "db/MojDbServiceDefs.h"
#include "db/MojDbServiceHandler.h"

#ifndef MOJ_VERSION_STRING
#define MOJ_VERSION_STRING NULL
#endif

const MojChar* const MojDbLunaServiceApp::VersionString = MOJ_VERSION_STRING;

int main(int argc, char** argv)
{
   MojAutoPtr<MojDbLunaServiceApp> app(new MojDbLunaServiceApp);
   MojAllocCheck(app.get());
   return app->main(argc, argv);
}

MojDbLunaServiceApp::MojDbLunaServiceApp()
: MojReactorApp<MojGmainReactor>(MajorVersion, MinorVersion, VersionString),
  m_mainService(m_dispatcher),
  m_idleTimeoutSignalHandler(this),
  m_slot(&m_idleTimeoutSignalHandler, &MojDbLunaServiceApp::IdleTimeoutSignalHandler::handleLunaIdleTimeout)
{
#ifdef WANT_DYNAMIC
    m_mainService.service().connectIdleTimeoutSignal(m_slot);
#endif
}

MojDbLunaServiceApp::~MojDbLunaServiceApp()
{
}

MojErr MojDbLunaServiceApp::init()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = Base::init();
	MojErrCheck(err);

    MojDbStorageEngine::createEnv(m_env);
    MojAllocCheck(m_env.get());

    m_internalHandler.reset(new MojDbServiceHandlerInternal(m_mainService.db(), m_reactor, m_mainService.service()));
    MojAllocCheck(m_internalHandler.get());

    err = m_mainService.init(m_reactor);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbLunaServiceApp::configure(const MojObject& conf)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err = Base::configure(conf);
    MojErrCheck(err);

    m_conf = conf;

    (void) conf.get(MojDbStorageEngine::engineFactory()->name(), m_engineConf);
    err = m_env->configure(m_engineConf);
    MojErrCheck(err);
    err = m_mainService.db().configure(conf);
    MojErrCheck(err);

    MojObject dbConf;
    err = conf.getRequired("db", dbConf);
    MojErrCheck(err);

    MojObject dbPath;
    err = dbConf.getRequired("path", dbPath);
    MojErrCheck(err);
    err = dbPath.stringValue(m_dbDir);
    MojErrCheck(err);

    MojObject serviceName;
    err = dbConf.getRequired("service_name", serviceName);
    MojErrCheck(err);
    err = serviceName.stringValue(m_serviceName);
    MojErrCheck(err);

    err = m_mainService.configure(dbConf);
    MojErrCheck(err);

#ifdef WANT_DYNAMIC
    MojUInt32 idleTimeOut = 0;
    bool found = false;
    err = dbConf.get(_T("dynamicIdleTimeoutSec"), idleTimeOut, found);
    MojErrCheck(err);
    if (found && (idleTimeOut > 0))
    {
        m_mainService.service().setIdleTimeout(idleTimeOut);
    }
#endif
    return MojErrNone;
}

MojErr MojDbLunaServiceApp::open()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    LOG_DEBUG("[mojodb] starting...");

	MojErr err = Base::open();
	MojErrCheck(err);

	// start message queue thread pool
	err = m_dispatcher.start(NumThreads);
	MojErrCheck(err);

	// open db env
	bool dbOpenFailed = false;
	err = m_env->open(m_dbDir);
	MojErrCatchAll(err) {
        MojString error;
        MojErrCheck(MojErrToString(err, error));

        LOG_ERROR(MSGID_LUNA_SERVICE_DB_OPEN,
                  1,
                  PMLOGKS("dbdata", error.data()),
                  "Can't init env");
        MojErrThrow(err);   // we should stop as soon as possible
	}

	// open db services
    err = m_mainService.open(m_reactor, m_env.get(), m_serviceName, m_dbDir, m_engineConf);
    MojErrCatchAll(err) {
        MojString error;
        MojErrCheck(MojErrToString(err, error));

        LOG_ERROR(MSGID_LUNA_SERVICE_DB_OPEN,
                  1,
                  PMLOGKS("dbdata", error.data()),
                  "Can't init service main");

        MojErrThrow(err);   // we should stop as soon as possible
	}

	// open internal handler
	err = m_internalHandler->open();
	MojErrCheck(err);
	err = m_mainService.service().addCategory(MojDbServiceDefs::InternalCategory, m_internalHandler.get());
	MojErrCheck(err);
	err = m_internalHandler->configure(m_conf, dbOpenFailed);
	MojErrCheck(err);

    err = m_internalHandler->subscribe();
    MojErrCheck(err);

    LOG_DEBUG("[mojodb] started");

	return MojErrNone;
}

MojErr MojDbLunaServiceApp::close()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    LOG_DEBUG("[mojodb] stopping...%s",m_serviceName.data());

	// stop dispatcher
	MojErr err = MojErrNone;
	MojErr errClose = m_dispatcher.stop();
	MojErrAccumulate(err, errClose);
	errClose = m_dispatcher.wait();
	MojErrAccumulate(err, errClose);
	// close services
	errClose = m_mainService.close();
	MojErrAccumulate(err, errClose);

	m_internalHandler->close();
	m_internalHandler.reset();

	errClose = Base::close();
	MojErrAccumulate(err, errClose);

    LOG_DEBUG("[mojodb] stopped %s",m_serviceName.data());

	return err;
}

MojErr MojDbLunaServiceApp::handleArgs(const StringVec& args)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = Base::handleArgs(args);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLunaServiceApp::displayUsage()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err = displayMessage(_T("Usage: %s [OPTION]...\n"), name().data());
    MojErrCheck(err);

	return MojErrNone;
}

// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#ifndef FAKELSMESSAGE_H
#define FAKELSMESSAGE_H

#include <luna-service2/lunaservice.h>

struct LSMessage {
    int          ref;           //< refcount

    void *transport_msg; //< underlying transport message
    LSHandle    *sh;            //< connection to bus used by message.
    const char  *category;      //< cache of category from message
    const char  *method;        //< cache of method from message
    char  *methodAllocated;    //< set if method was allocated string
    const char  *payload;       //< cache of the payload from message
    char  *payloadAllocated;    //< set if payload was allocated string
    char  *uniqueTokenAllocated; //< unique token of message
    char  *kindAllocated;    //< kind (cat+method) of message
    void      *_json_object;   //<  deprecated, but left for binary compatibility
    LSMessageToken responseToken; //< cache of the response token
                                  // of the message.
                                  // For signals, this is the original
                                  // LSSignalCall() token.
    bool         ignore;
    bool         serviceDownMessage;
};

#endif // FAKELSMESSAGE_H

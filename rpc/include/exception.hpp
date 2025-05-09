/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

 


/**
* @defgroup devicesettings
* @{
* @defgroup rpc
* @{
**/


#ifndef _DS_MGR_EXCEPTION_H_
#define _DS_MGR_EXCEPTION_H_
#ifndef _DS_EXCEPTION_H_
#define _DS_EXCEPTION_H_

#include <string>
#include <iostream>
#include <exception>

namespace device {

class Exception : public std::exception {
	int _err;
	std::string _msg;

public:
	Exception(const char *msg = "No Message for this exception") throw() : _msg(msg), _err(0){
	}

	Exception(int err, const char *msg = "No Message for this Exception") throw()
	: _err(err), _msg(msg){
	};
	virtual const std::string & getMessage() const {
		return _msg;
	}
	virtual int getCode() const {
		return _err;
	}
	virtual const char * what() const throw() {
		return _msg.c_str();
	}

	virtual ~Exception() throw() {};
};

}
#endif /* EXCEPTION_H_ */
#endif


/** @} */
/** @} */

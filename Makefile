##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
all:
ifneq ($(STANDALONE_BUILD_ENABLED),y)
	@make -C hal/src uninstall clean all
endif
	@make -C rpc/cli uninstall clean all
	@make -C rpc/srv uninstall clean all
	@make -C ds uninstall clean all
	@make -C hal-configs uninstall clean all
	#@make -C sample uninstall clean all

clean:
	@make -C rpc/srv $@ 
	@make -C rpc/cli $@ 
	@make -C ds $@
	@make -C hal-configs $@
ifneq ($(STANDALONE_BUILD_ENABLED),y)
	@make -C hal/src $@
endif
#	@make -C sample $@
	@make -C test $@

uninstall: clean
	@make -C rpc/srv $@ 
	@make -C rpc/cli $@ 
	@make -C ds $@
	@make -C hal-configs $@
	@make -C hal/emu $@
	@make -C hal/rng150 $@
#	@make -C sample $@
	@make -C test $@

unittest: 
	@make -C test clean all


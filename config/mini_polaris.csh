setenv MINI_POLARIS_LOCAL ${HOME}/minipolaris
setenv MINI_POLARIS_ROOT /pub/courses/cpsc605/minipolaris 
if $?LD_LIBRARY_PATH then
  setenv LD_LIBRARY_PATH ${MINI_POLARIS_LOCAL}/shared_libs:${MINI_POLARIS_ROOT}/cvdl/shared_libs:${LD_LIBRARY_PATH}
else
  setenv LD_LIBRARY_PATH ${MINI_POLARIS_LOCAL}/shared_libs:${MINI_POLARIS_ROOT}/cvdl/shared_libs
endif
setenv PATH ${MINI_POLARIS_LOCAL}:${MINI_POLARIS_ROOT}/bin/i686-pc-linux-gnu:${MINI_POLARIS_ROOT}/cvdl:${PATH}


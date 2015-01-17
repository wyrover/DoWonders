#!/bin/sh

cd Wonders98
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then exit 1; fi

cd WondersMe
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then exit 1; fi

cd Wonders2000
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then exit 1; fi

cd WondersXP
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then exit 1; fi

cd WondersVista
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then exit 1; fi

cd Wonders7
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then exit 1; fi

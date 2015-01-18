#!/bin/sh

cd Wonders2000
if do-gcc-64.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd WondersXP
if do-gcc-64.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd WondersVista
if do-gcc-64.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd Wonders7
if do-gcc-64.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

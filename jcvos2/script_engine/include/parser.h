#pragma once

#include "iplugin.h"

namespace jcscript
{
	bool CreateVarOp(jcvos::IValue * val, IAtomOperate * &op);
	bool Parse(IPluginContainer * plugin_container, LSyntaxErrorHandler * err_handler, LPCTSTR &str, LPCTSTR last, IAtomOperate * & script);
	bool Parse(IPluginContainer * plugin_container, LSyntaxErrorHandler * err_handler, FILE * file, IAtomOperate * & script);
	bool Parse(jcvos::IJCStream * stream);

};
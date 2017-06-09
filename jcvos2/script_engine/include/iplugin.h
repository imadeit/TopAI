﻿#pragma once

#include "../../jcparam/include/ivalue.h"

// 定义一个基本执行单位，包括从何处取参数1和2，执行什么过程(IProxy)，经结果保存在何处。
// 如果执行的过程为NULL，则直接将参数1保存到结果

namespace jcscript
{
	enum OP_PROP
	{
		OPP_LOOP_SOURCE = 0x80000000,
		OPP_LOOP_DEPEND = 0x40000000,
		OOP_STATE =		  0x20000000,	// a op which need to keep state (state machine)
	};

	class CExitException
	{
	};	

	class IScript : virtual public IJCInterface
	{
	};

	class LSyntaxErrorHandler
	{
	public:
		virtual void OnError(JCSIZE line, JCSIZE column, LPCTSTR msg) = 0;
	};

	class IAtomOperate : public IJCInterface
	{
	public:
		virtual bool GetResult(jcvos::IValue * & val) = 0;
		virtual bool Invoke(void) = 0;
		virtual void SetSource(UINT src_id, IAtomOperate * op) = 0;
		virtual UINT GetProperty(void) const = 0;
		// dependency：用于表示命令之间的依赖性。
		//	常数的dependency=0，全局变量：1，其他operate继承其source中最大的dependency.
		//	当每经过一次loop source或者inport后，dependency+1。
		//	所有的sequence的派生类都有自己的depency，当operate的dependency小于sequence的dependency时，
		//	可以将operate提取到其上一级sequence中，从而实现优化。
		virtual UINT GetDependency(void) = 0;
	// 用于检查编译结果
	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile) = 0;
	};

	class IOutPort: virtual public IAtomOperate
	{
	public:
		// 如果队列空，返回false，并且val=NULL
		virtual bool PopupResult(jcvos::IValue * & val) = 0;
		// 如果队列满，返回false
		virtual bool PushResult(jcvos::IValue * val) = 0;
		virtual bool IsEmpty(void) = 0;
	};

	//class IHelpMessage : virtual public IJCInterface
	//{
	//public:
	//	virtual void HelpMessage(FILE * output) = 0;
	//};

	class IFeature : virtual public IJCInterface
	{
	public:
		virtual bool Invoke(jcvos::IValue * row, IOutPort * outport) = 0;
		virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const = 0;
		virtual bool Clean(void) = 0;
		virtual bool PushParameter(const CJCStringT & var_name, jcvos::IValue * val) = 0;
		virtual bool CheckAbbrev(TCHAR param_name, CJCStringT & var_name) const = 0;

		virtual LPCTSTR GetFeatureName(void) const = 0;
	};

	class IPlugin : public virtual IJCInterface
	{
	public:
		virtual bool Reset(void) = 0; 
		virtual void GetFeature(const CJCStringT & cmd_name, IFeature * & pr) = 0;
		virtual void ShowFunctionList(FILE * output) const = 0;

	public:
		virtual LPCTSTR name() const = 0;
	};

	class IPluginContainer : public virtual IJCInterface
	{
	public:
		virtual bool GetPlugin(const CJCStringT & name, IPlugin * & plugin) = 0;
		virtual void GetVarOp(IAtomOperate * & op) = 0;
		virtual bool RegistPlugin(IPlugin * plugin) = 0;
	};
};




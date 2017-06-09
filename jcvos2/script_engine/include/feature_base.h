﻿#pragma once

#include <jcparam.h>
#include "iplugin.h"

class CPluginManager;

///////////////////////////////////////////////////////////////////////////////
//-- CParamDef
class CParamDef
{
public:
	CParamDef(LPCTSTR name, TCHAR abbr, JCSIZE size, JCSIZE offset, LPCTSTR desc)
		: m_name(name), m_abbr(abbr), m_size(size), m_offset(offset)
		, m_description(desc)
	{ }

	LPCTSTR name() const { return m_name;}
	virtual void SetValue(BYTE * base, jcvos::IValue * val) const = 0;

public:
	LPCTSTR m_name;
	TCHAR	m_abbr;
	JCSIZE	m_size;
	JCSIZE	m_offset;
	LPCTSTR	m_description;
};

class CAbbrevIndex
{
public:
	CAbbrevIndex(void) {memset(m_abbr_map, 0, 128 * sizeof (CParamDef*)); }
	~CAbbrevIndex(void) { }

public:
	void push_back(const CParamDef * def)
	{
		JCASSERT(def);
		TCHAR abbr = (def->m_abbr & 0x7F);
		if (0 == abbr) return;
		if (m_abbr_map[abbr]) 
		{
			THROW_ERROR( ERR_PARAMETER, 		// 略称重复定义
				_T("Abbreviation %c has already been used by argument %s"), def->m_abbr, 
				def->m_name);
		}
		m_abbr_map[abbr] = def;
	}

	JCSIZE size(void) const {return 128;}

	const CParamDef * operator [] (JCSIZE abbr)
	{
		return m_abbr_map[abbr];
	}

	void reserve(JCSIZE) {};

protected:
	const CParamDef * m_abbr_map[128];
};

typedef jcvos::CStringTable<CParamDef, CAbbrevIndex> CParamDefTab;

template <typename VAL_TYPE>
class CTypedParamDef : public CParamDef
{
public:
	CTypedParamDef(LPCTSTR name, TCHAR abbr, JCSIZE offset, LPCTSTR desc = NULL)
		: CParamDef(name, abbr, sizeof(VAL_TYPE), offset, desc)
	{ }

	virtual void SetValue(BYTE * base, jcvos::IValue * val) const
	{
		CJCStringT str_val;
		val->GetValueText(str_val);

		VAL_TYPE * ptr = (VAL_TYPE*)( base + m_offset);
		jcvos::CConvertor<VAL_TYPE>::S2T(str_val.c_str(), *ptr);
	}
};

///////////////////////////////////////////////////////////////////////////////
//-- CFeatureBase
// F_TYPE: feature type,
// P_TYPE: plugin type,
template <class F_TYPE, class P_TYPE>
class CFeatureBase
	:virtual public jcscript::IFeature
{
public:
	typedef CFeatureBase<F_TYPE, P_TYPE>  _BASE_TYPE;

protected:
	CFeatureBase(void) : m_init_(false) {}
	~CFeatureBase(void)  {}

public:
	static void Create(jcscript::IPlugin * plugin, jcscript::IFeature * & f)
	{	
		JCASSERT(NULL == f);
		F_TYPE * _f = new F_TYPE( );
		_f->m_plugin = dynamic_cast<P_TYPE *>(plugin);
		f = static_cast<jcscript::IFeature*>(_f);
	}

public:
	static LPCTSTR name(void) {return m_feature_name;} 
	virtual bool PushParameter(const CJCStringT & var_name, jcvos::IValue * val)
	{
		const CParamDef * def = m_param_def_tab.GetItem(var_name);
		if (!def) return false;
		F_TYPE * ptr = dynamic_cast<F_TYPE*>(this);
		def->SetValue((BYTE*)(ptr), val);
		return true;
	}

	virtual bool CheckAbbrev(TCHAR abbr, CJCStringT & var_name) const
	{
		const CParamDef * def = m_param_def_tab.GetItem(abbr);
		if (!def) return false;	
		var_name = def->m_name;
		return true;
	}
	
	virtual LPCTSTR GetFeatureName(void) const {return m_feature_name;}

	virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const {};
	virtual bool Clean(void) {return false; };
	virtual bool Init(void) {return true; };
	virtual bool InternalInvoke(jcvos::IValue * row, jcscript::IOutPort * outport) {return false;}

	virtual bool Invoke(jcvos::IValue * row, jcscript::IOutPort * outport)
	{
		if (!m_init_) m_init_ = Init();
		bool br = InternalInvoke(row, outport);
		if (!br) m_init_ = false;
		return br;
	}

protected:
	static CParamDefTab	m_param_def_tab;
	static LPCTSTR m_feature_name;
	P_TYPE		* m_plugin;
	bool m_init_;
};

///////////////////////////////////////////////////////////////////////////////
// -- Plugin base
typedef void (*FEATURE_CREATOR)(jcscript::IPlugin *, jcscript::IFeature * &);

class CFeatureInfo
{
public:
	CFeatureInfo(LPCTSTR name, FEATURE_CREATOR creator, LPCTSTR desc = NULL)
		: m_feature_name(name), m_creator(creator), m_description(desc)
	{};

public:
	LPCTSTR	m_feature_name;
	FEATURE_CREATOR	m_creator;
	LPCTSTR		m_description;

public:
	LPCTSTR name() const {return m_feature_name;}
};

template <class F>
class CTypedFtInfo : public CFeatureInfo
{
public:
	CTypedFtInfo(LPCTSTR desc = NULL)
		: CFeatureInfo(F::name(), &F::Create, desc)
	{};
};

typedef jcvos::CStringTable<CFeatureInfo> FEATURE_LIST;

template <class P_TYPE>
class CPluginBase2 
	: virtual public jcscript::IPlugin
	, public CJCInterfaceBase
{
public:
	virtual void GetFeature(const CJCStringT & feature_name, jcscript::IFeature * & ptr)
	{
		JCASSERT(NULL == ptr);
		const CFeatureInfo * info = m_feature_list.GetItem(feature_name);
		if (NULL == info) return;
		info->m_creator(static_cast<jcscript::IPlugin *>(this), ptr);
	}
	virtual void ShowFunctionList(FILE * output) const {}

	virtual LPCTSTR name() const {	return m_name.c_str();		}

	static bool Regist(CPluginManager * manager)
	{
		manager->RegistPlugin(
			m_name, NULL, CPluginManager::CPluginInfo::PIP_SINGLETONE, 
			CPluginBase2<P_TYPE>::Create);
		return true;
	}

	static bool Create(jcvos::IValue * param, jcscript::IPlugin * &plugin)
	{
		JCASSERT(NULL == plugin);
		plugin = static_cast<jcscript::IPlugin*>(new P_TYPE);
		return true;
	}

protected:
	static CJCStringT m_name;
	static FEATURE_LIST	m_feature_list;
};

class CCategoryComm :
	virtual public jcscript::IPlugin,
	public CJCInterfaceBase
{
public:
	CCategoryComm(LPCTSTR name);
	virtual ~CCategoryComm(void);

public:
	virtual bool Reset(void) {return false;}; 
	virtual void GetFeature(const CJCStringT & feature_name, jcscript::IFeature * & ptr);
	virtual void ShowFunctionList(FILE * output) const;
	virtual void RegisterFeature(LPCTSTR feature_name, FEATURE_CREATOR creator, LPCTSTR desc = NULL);
	//static bool Regist(CPluginManager * manager)
	//static bool Create(jcvos::IValue * param, jcscript::IPlugin * &plugin);
	virtual LPCTSTR name() const {return m_name.c_str();};
	
	template <class FEATURE>
	void RegisterFeatureT(void)
	{
		RegisterFeature(FEATURE::name(), &FEATURE::Create, FEATURE::desc());
	}

protected:
	CJCStringT m_name;
	FEATURE_LIST	m_feature_list;
};
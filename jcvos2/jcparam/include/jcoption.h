#pragma once

#include <map>
//#include "../../CommLib/CoolCtrl/CoolCtrl.h"
//#include "../../CommLib/OITL/oitl.h"

#include "parameter_if.h"
#include "parameter_impl.h"

class COptionBase;

class IOptionPage
{
public:
	virtual bool Construct(DWORD dwStyle, const RECT & rect, HWND hwnd, UINT nID) = 0;
    virtual bool Attach(void *) = 0;
    virtual bool Detach(void) = 0;
    virtual bool Destory() = 0;
    virtual void UpdateData() = 0;
    virtual void RetrievalData() = 0;
};

class IOptionEnumObserver
{
public:
    virtual void HandleEnumOption(COptionBase * option) = 0;
};

typedef COptionBase * (OPTION_FACTORY)(const CJCStringT & name, COptionBase * parent);

class OptionDefine
{
public:
    CJCStringT             m_option_name;
    OPTION_FACTORY      * m_factory;
    CJCStringT             m_default_value;
};

///////////////////////////////////////////////////////////////////////////////
// -- 
class COptionBase
{
public:
    enum OPTION_TYPE
    {
        OPTION_SET,
        OPTION_ITEM,
    };

    virtual OPTION_TYPE GetType() const = 0;
    virtual LPCTSTR GetOptionName() const = 0;
    virtual void Delete(void) = 0;
    virtual void EnumSubOptions(IOptionEnumObserver * observer, bool recursive) = 0;
    virtual COptionBase * GetParent() const = 0;

    virtual IOptionPage * GetPageGui() = 0;
    //virtual IOptionPage * CreatePageGui() = 0;
    //virtual void DeletePageGui() = 0;
};



///////////////////////////////////////////////////////////////////////////////
// -- 

class COptionItemBase : public COptionBase
{
public:
    COptionItemBase(COptionBase * parent) : m_parent(parent) {}
public:
    virtual OPTION_TYPE GetType() const { return OPTION_SET; }    
    virtual void SetValue(const CJCStringT & value) = 0;
    virtual void EnumSubOptions(IOptionEnumObserver * observer, bool recursive)
    {
        JCASSERT(observer);
        observer->HandleEnumOption( static_cast<COptionBase*>(this) );
    }
    virtual COptionBase * GetParent() const { return m_parent; }
    virtual IOptionPage * GetPageGui() { return NULL; }
    virtual IOptionPage * CreatePageGui() { return NULL; }
    virtual void DeletePageGui() {}

protected:
    COptionBase * m_parent;
};


template <typename PARAM_TYPE>
class COptionItem   : public virtual PARAM_TYPE
                    , public virtual COptionItemBase
{
public:
    static COptionBase * CreateOptionItem(const CJCStringT & name, COptionBase * parent)
    {
        COptionBase * option = static_cast<COptionBase*>(
            new COptionItem<PARAM_TYPE>(name, parent) );
        return option;
    }

public:
    COptionItem(const CJCStringT & name, COptionBase * parent) 
        : PARAM_TYPE(name.c_str()), COptionItemBase(parent) {}
    virtual ~COptionItem(void)    {}

public:
    virtual void Delete(void)   { PARAM_TYPE::Release(); }
    virtual LPCTSTR GetOptionName() const
    {
        return PARAM_TYPE::GetParamName();
    }
    virtual void SetValue(const CJCStringT & value)
    {
        PARAM_TYPE::SetValueText(value.c_str());
    }
};

///////////////////////////////////////////////////////////////////////////////
// -- 

class COptionSet : public COptionBase
{
public:
    typedef std::map<CJCStringT, COptionBase *> OptionMap;
    typedef std::pair<CJCStringT, COptionBase *> OptionPare;

public:
    //static COptionBase * CreateOptionSet(const CJCStringT & name, COptionBase * parent);

public:
    COptionSet(void);
    COptionSet(const CJCStringT & name, COptionBase * parent);
    virtual ~COptionSet(void);
    virtual void Delete(void);

public:
    // 通过名称，返回option的指针。如果该名称不存在，则返回其父节点的指针，并且将local_name贼name中返回。
    //  如果没有对应的父节点，则返回NULL
    virtual COptionBase * QuerySubItemByName(const CJCStringT & name);
    virtual bool AddSubItem(COptionBase * option);
    virtual OPTION_TYPE GetType() const { return OPTION_SET; }
    virtual LPCTSTR GetOptionName() const { return m_name.c_str(); }
    virtual void EnumSubOptions(IOptionEnumObserver * observer, bool recursive);
    virtual COptionBase * GetParent() const { return m_parent; }

    //virtual void InitializeUpdateData() = 0;

protected:
    OptionMap   m_sub_options;
    CJCStringT     m_name;
    COptionBase * m_parent;

public:
    void UpdateData(void);
    virtual void SetItemValue(IParameter * param);
};

///////////////////////////////////////////////////////////////////////////////
// -- 
template <typename OPTION_PAGE>
class COptionSetGui/*     : public OPTION_PAGE*/
    : public COptionSet
{
public:
    static COptionBase * CreateOptionSetGui(const CJCStringT & name, COptionBase * parent)
    {
        COptionBase * option = static_cast<COptionBase*>(
            new COptionSetGui<OPTION_PAGE>(name, parent) );
        return option;
    }

public:
    COptionSetGui(const CJCStringT & name, COptionBase * parent)
        : COptionSet(name, parent) {}
    virtual ~COptionSetGui(void) {}

    virtual IOptionPage * GetPageGui() { 
        m_page_gui.SetOptionSet(static_cast<COptionSet *>(this) );
        return static_cast<IOptionPage*>(&m_page_gui); }

    virtual IOptionPage * CreatePageGui() {
        m_page_gui.SetOptionSet(static_cast<COptionSet *>(this) );
        return static_cast<IOptionPage*>(&m_page_gui); }

    virtual void DeletePageGui() {}

//public: 
//    virtual void InitializeUpdateData() = 0;


protected:
    OPTION_PAGE     m_page_gui;
};

///////////////////////////////////////////////////////////////////////////////
// -- 





#define NAME_SEPERATOR  _T('.')

class COptionManager    : public COptionSet
                        //, public IOptionEnumObserver
{
public:
    COptionManager(void);
    virtual ~COptionManager(void);
    void Initialize(const OptionDefine * option_define_list, int option_define_size);
    void Uninitialize(void);

protected:
    //OptionMap     m_option_set_map;

public:
    void SaveToFile(LPCTSTR filename);
    void LoadFromFile(LPCTSTR filename);
    virtual IOptionPage * GetPageGui() { return NULL; }
    virtual IOptionPage * CreatePageGui() { return NULL; }
    virtual void DeletePageGui() {}

    // 解析选项的名称，按照"."将名称分为item_name和parent_name，parent_name通过输入参数返回
    //bool ParseOptionName(CJCStringT & full_name, CJCStringT & item_name);

    //virtual COptionBase * QuerySubItemByName(CJCStringT & name);

    //virtual void HandleEnumOption(COptionBase * option);
    //void ShowOptionGui(void);
    //virtual IOptionPage * GetPageGui() { return NULL;}
    //virtual IOptionPage * CreatePageGui() {return NULL;}
    //virtual void DeletePageGui() {return NULL;}

};

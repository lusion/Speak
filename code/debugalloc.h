#ifdef MEM_DEBUG
// debugalloc.h: Debug Extension code
// See debugalloc.cpp for usage and purpose
// 
/*******************************************************************
//		*** COPYRIGHT NOTICE ****
//
//	Copyright 2003. David Maisonave (www.axter.com)
//	
//	This code is free for use under the following conditions:
//
//		You may not claim authorship of this code.
//		You may not sell or distrubute this code without author's permission.
//		You can use this code for your applications as you see fit, but this code may not be sold in any form
//		to others for use in their applications.
//		This copyright notice may not be removed.
//
//
//  No guarantees or warranties are expressed or implied. 
//	This code may contain bugs.
*******************************************************************/
//  Please let me know of any bugs, bug fixes, or enhancements made to this code.
//  If you have ideas for this, and/or tips or admonitions, I would be glad to hear them.
//
//////////////////////////////////////////////////////////////////////


#ifndef DEBUGALLOC_LOG_H_
#define DEBUGALLOC_LOG_H_

#define USE_DEBUG_MEM_ALLOCATION

#if (defined(DEBUG) || defined (_DEBUG))
#define DEBUG_VERSION_
#endif

#ifdef WIN32
#ifndef __FUNCTION__ //The GNU compiler and VC++ 7.x supports this macro
#define __FUNCTION__ ""  //If compiler does not support it, then use empty string
#endif //!__FUNCTION__
#endif //WIN32

#if defined(DEBUG_VERSION_) && defined(USE_DEBUG_MEM_ALLOCATION) && !defined(DEBUG_LOG_INCLUDE_FROM_DEBUGLOGGER_CPP_)
void operator delete( void *pvMem );
#endif

namespace debug_ext
{
//Heap allocation debugging code
class debug_mem_allocation
{
public:
	enum ACTION_SWITCH{AllocateMem, DeAllocMem, GenerateReport};
    debug_mem_allocation(const char* FileName, int LineNo, const char* FunctionName)
		:m_FileName(FileName), m_LineNo(LineNo), m_FuncName(FunctionName){}
    template<typename T>
		void operator-(T &ptr)
    {
		Log(ptr, DeAllocMem);
		delete ptr;
		ptr = NULL;
    }
    template<typename T>
		void operator-(const T &ptr)
    {
		Log(ptr, DeAllocMem);
		delete ptr;
    }
    template<typename T>
		void operator/(T &ptr)
    {
		Log(ptr, DeAllocMem);
		delete [] ptr;
		ptr = NULL;
    }
    template<typename T>
		T& operator<<(T &ptr)
    {
		Log(ptr, AllocateMem);
		return ptr;
    }
    template<typename T>
		const T& operator<<(const T &ptr)
    {
		Log(ptr, AllocateMem);
		return ptr;
    }
	template<typename T>
	void free_and_nullify(T &memblock , const char* VariableName)
	{
		free_and_log(memblock);
		memblock = NULL;
	}
	void free_and_log(void *memblock);
	void *malloc_and_log(int size );
	void *realloc_and_log(void *memblock, int size );
	static void SendOutCurrentLog();
	void SaveMe();
private:
	void Log(void const * const Ptr, ACTION_SWITCH Sw);
    const char* m_FileName;
    const int m_LineNo;
    const char* m_FuncName;
};

#if defined(DEBUG_VERSION_) && defined(USE_DEBUG_MEM_ALLOCATION) && !defined(DEBUG_LOG_INCLUDE_FROM_DEBUGLOGGER_CPP_)
#define new							(0)?NULL:debug_ext::debug_mem_allocation(__FILE__, __LINE__, __FUNCTION__)<< new 
#ifdef DEBUGALLOC_MULTITHREAD_
#define delete						debug_ext::debug_mem_allocation(__FILE__, __LINE__, __FUNCTION__)-
#define DELETE_ARRAY_AND_NULLIFY	debug_ext::debug_mem_allocation(__FILE__, __LINE__, __FUNCTION__)/
#else
#define delete						debug_ext::debug_mem_allocation(__FILE__, __LINE__, __FUNCTION__).SaveMe(), delete
#endif //DEBUGALLOC_MULTITHREAD_
#define malloc(x)					debug_ext::debug_mem_allocation(__FILE__, __LINE__, __FUNCTION__).malloc_and_log(x)
#define realloc(m,s)				debug_ext::debug_mem_allocation(__FILE__, __LINE__, __FUNCTION__).realloc_and_log(m,s)
#define free(x)						debug_ext::debug_mem_allocation(__FILE__, __LINE__, __FUNCTION__).free_and_nullify(x, #x)
#else
#define DELETE_ARRAY_AND_NULLIFY	delete []
#endif


}//End of namespace debug_ext

#endif //!DEBUGALLOC_LOG_H_

#endif

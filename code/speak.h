#ifndef __SPEAK_H
#define __SPEAK_H

#include "channel.h"
#include "config.h"
#include <string>


using namespace std;

struct eqstr;
typedef pair<string, IChannel *> channel_pair;

#ifdef HASH_MAP
namespace __gnu_cxx
{                                                                                             
  template<> struct hash< std::string >                                                       
  {                                                                                           
    size_t operator()( const std::string& x ) const                                           
    {                                                                                         
      return hash< const char* >()( x.c_str() );                                              
    }                                                                                         
  };                                                                                          
};

typedef __gnu_cxx::hash_map<string, IChannel *> channel_map;
#else
typedef unordered_map<string, IChannel *> channel_map;
#endif



class Speak {
	public:
		Speak(void);
		void addChannel(const char *name, IChannel *channel);
		void addSpecialChannel(IChannel *c, char *name);
		IChannel *getChannel(const char *name);

		void uniqueName(char *name);

	private:
		channel_map channels;
};

extern Speak speak;

#endif

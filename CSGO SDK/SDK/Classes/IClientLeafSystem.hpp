#pragma once
#include "sdk.hpp"

class ClientRenderHandle_t;
class CClientLeafSubSystemData;
using RenderableModelType_t = int;
class SetupRenderInfo_t;
class ClientLeafShadowHandle_t;
class ClientShadowHandle_t;

struct RenderableInstance_t {
    uint8_t m_alpha;
    __forceinline RenderableInstance_t() : m_alpha{ 255ui8 } {}
};

class IClientLeafSystem {
public:
  
};

typedef int TABLEID;

class INetworkStringTable;

typedef void( *pfnStringChanged )( void* object, INetworkStringTable* stringTable, int stringNumber, char const* newString, void const* newData );

class INetworkStringTable
{
public:

    virtual					~INetworkStringTable( void ) {};

    // Table Info
    virtual const char* GetTableName( void ) const = 0;
    virtual TABLEID			GetTableId( void ) const = 0;
    virtual int				GetNumStrings( void ) const = 0;
    virtual int				GetMaxStrings( void ) const = 0;
    virtual int				GetEntryBits( void ) const = 0;

    // Networking
    virtual void			SetTick( int tick ) = 0;
    virtual bool			ChangedSinceTick( int tick ) const = 0;

    // Accessors (length -1 means don't change user data if string already exits)
    virtual int				AddString( bool bIsServer, const char* value, int length = -1, const void* userdata = 0 ) = 0;

    virtual const char* GetString( int stringNumber ) = 0;
    virtual void			SetStringUserData( int stringNumber, int length, const void* userdata ) = 0;
    virtual const void* GetStringUserData( int stringNumber, int* length ) = 0;
    virtual int				FindStringIndex( char const* string ) = 0; // returns INVALID_STRING_INDEX if not found

                                                                     // Callbacks
    virtual void			SetStringChangedCallback( void* object, pfnStringChanged changeFunc ) = 0;
};

class CNetworkStringTableContainer
{
public:
    INetworkStringTable* FindTable( const char* tableName )
    {
        typedef INetworkStringTable* ( __thiscall* oFindTable )( PVOID, const char* );
        return Memory::VCall< oFindTable >( this, 3 )( this, tableName );
    }
};
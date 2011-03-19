
/***************************************************************************
 *  blackboard.cpp - BlackBoard Interface
 *
 *  Created: Sat Sep 16 17:11:13 2006 (on train to Cologne)
 *  Copyright  2006-2008  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <blackboard/blackboard.h>

#include <string>
#include <cstring>

namespace fawkes {

/** @class BlackBoard blackboard.h <blackboard/blackboard.h>
 * The BlackBoard.
 * This class is the single one entry point for programs that use the BlackBoard.
 * It is used to open and close interfaces, register and unregister listeners and
 * observers and to maintain the BlackBoard shared memory segment. Not other classes
 * shall be used directly.
 *
 * The BlackBoard holds a number of so-called interfaces. The interfaces store
 * data and provide means to pass messages. The BlackBoard also allows for registering
 * listeners and observers. The listeners can be used to get events for specific
 * interfaces while the observer gets global interface creation and destruction
 * events for a specified set of types of interfaces.

 * An interface consists of a few parts. First there is the storage block. This
 * is a chunk of memory in the shared memory segment where the actual data is stored.
 * Then there is the accessor object, an instance of a derivate of the Interface
 * class which is used to access the data in the shared memory segment. Last but
 * not least there is an internal message queue that can be used to pass messages
 * from readers to the writer (not the other way around!).
 *
 * The interface manager keeps track of all the allocated interfaces. Events
 * can be triggered if a specific interface changes (like logging the data to
 * a file, sending it over the network or notifying another interface of such
 * a change).
 *
 * Interfaces can only be instantiated through the BlackBoard. The BlackBoard
 * instantiates an interface on request and guarantees that the instance
 * is fully initialized and usable. This cannot be guaranteed if instantiating an
 * interface through any other means!
 *
 * Interfaces can be opened for reading or writing, not both! There can be only
 * one writer at a time for any given interface. Interfaces are identified via a
 * type (which denotes the data and its semantics) and an identifier. There may
 * be several interfaces for a given type, but the identifier has to be unique.
 * The identifier is in most cases a well-known string that is used to share data
 * among plugins.
 *
 * Interfaces provide a way to propagate data to the writer via messages. Available
 * messages types depend on the interface type. Only matching messages are accepted
 * and can be queued.
 *
 * The BlackBoard can operate in two modes, master and slave. Only the master
 * creates and destroys the shared memory segment. Currently, the slave mode is not
 * fully implemented and thus may not be used.
 *
 * @see Interface
 * @see Message
 *
 * @author Tim Niemueller
 *
 *
 * @fn Interface *  BlackBoard::open_for_reading(const char *type, const char *identifier)
 * Open interface for reading.
 * This will create a new interface instance of the given type. The result can be
 * casted to the appropriate type.
 * @param type type of the interface
 * @param identifier identifier of the interface
 * @return new fully initialized interface instance of requested type
 * @exception OutOfMemoryException thrown if there is not enough free space for
 * the requested interface.
 *
 *
 * @fn Interface *  BlackBoard::open_for_writing(const char *type, const char *identifier)
 * Open interface for writing.
 * This will create a new interface instance of the given type. The result can be
 * casted to the appropriate type. This will only succeed if there is not already
 * a writer for the given interface type/id!
 * @param type type of the interface
 * @param identifier identifier of the interface
 * @return new fully initialized interface instance of requested type
 * @exception OutOfMemoryException thrown if there is not enough free space for
 * the requested interface.
 * @exception BlackBoardWriterActiveException thrown if there is already a writing
 * instance with the same type/id
 *
 *
 * @fn void BlackBoard::close(Interface *interface)
 * Close interface.
 * @param interface interface to close
 *
 *
 * @fn bool BlackBoard::is_alive() const throw() = 0
 * Check if the BlackBoard is still alive.
 * @return true, if the BlackBoard is still alive and may be used, false otherwise.
 *
 * @fn bool BlackBoard::try_aliveness_restore() throw()
 * Try to restore the aliveness of the BlackBoard instance.
 * Note that even though the aliveness of the BlackBoard is restored single
 * interfaces may still be invalid. That can for instance happen if a remote
 * connection is re-established and a writer has been created during the
 * downtime and an own writer instance of that very interface cannot be restored.
 * @return true if the aliveness could be restored and the BlackBoard is
 * operational again, false otherwise.
 *
 * @fn std::list<Interface *>  BlackBoard::open_multiple_for_reading(const char *type_pattern, const char *id_pattern = "*")
 * Open multiple interfaces for reading.
 * This will create interface instances for currently registered interfaces of
 * the given type that match the given ID pattern. The result can be casted to
 * the appropriate type.
 * @param type_pattern pattern of interface types to open, supports wildcards
 * similar to filenames (*, ?, []), see "man fnmatch" for all supported.
 * @param id_pattern pattern of interface IDs to open, supports wildcards similar
 * to filenames (*, ?, []), see "man fnmatch" for all supported.
 * @return list of new fully initialized interface instances of requested type.
 * You have to close all interfaces on your own when done with the list!
 *
 * @fn InterfaceInfoList * BlackBoard::list_all()
 * Get list of all currently existing interfaces.
 * @return list of interfaces
 *
 * @fn InterfaceInfoList * BlackBoard::list(const char *type_pattern, const char *id_pattern)
 * Get list of interfaces matching type and ID patterns.
 * See the fnmatch() documentation for possible patterns.
 * @param type_pattern pattern with shell like globs (* for any number of
 * characters, ? for exactly one character) to match the interface type.
 * @param id_pattern pattern with shell like globs (* for any number of
 * characters, ? for exactly one character) to match the interface ID.
 * @return list of interfaces
 *
 * @fn void BlackBoard::register_listener(BlackBoardInterfaceListener *listener, unsigned int flags)
 * Register BB event listener.
 * @param listener BlackBoard event listener to register
 * @param flags an or'ed combination of BBIL_FLAG_DATA, BBIL_FLAG_READER, BBIL_FLAG_WRITER
 * and BBIL_FLAG_INTERFACE. Only for the given types the event listener is registered.
 * BBIL_FLAG_ALL can be supplied to register for all events.
 *
 *
 * @fn void BlackBoard::unregister_listener(BlackBoardInterfaceListener *listener)
 * Unregister BB interface listener.
 * This will remove the given BlackBoard interface listener from any event that it was
 * previously registered for.
 * @param listener BlackBoard event listener to remove
 *
 *
 * @fn void BlackBoard::register_observer(BlackBoardInterfaceObserver *observer, unsigned int flags)
 * Register BB interface observer.
 * @param observer BlackBoard interface observer to register
 * @param flags an or'ed combination of BBIO_FLAG_CREATED, BBIO_FLAG_DESTROYED
 *
 *
 * @fn void BlackBoard::unregister_observer(BlackBoardInterfaceObserver *observer)
 * Unregister BB interface observer.
 * This will remove the given BlackBoard event listener from any event that it was
 * previously registered for.
 * @param observer BlackBoard event listener to remove
 */


/** Data changed notification flag. */
const unsigned int BlackBoard::BBIL_FLAG_DATA      = 1;
/** Message received notification flag. */
const unsigned int BlackBoard::BBIL_FLAG_MESSAGES  = 2;
/** Reader added/removed notification flag. */
const unsigned int BlackBoard::BBIL_FLAG_READER    = 4;
/** Writer added/removed notification flag. */
const unsigned int BlackBoard::BBIL_FLAG_WRITER    = 8;

/** All interface listener notifications. */
const unsigned int BlackBoard::BBIL_FLAG_ALL = 
  BBIL_FLAG_DATA | BBIL_FLAG_MESSAGES | BBIL_FLAG_READER | BBIL_FLAG_WRITER;

/** Interface creation notification flag. */
const unsigned int BlackBoard::BBIO_FLAG_CREATED   = 1;

/** Interface destruction notification flag. */
const unsigned int BlackBoard::BBIO_FLAG_DESTROYED = 2;

/** All interface observer notifications */
const unsigned int BlackBoard::BBIO_FLAG_ALL =
  BBIO_FLAG_CREATED | BBIO_FLAG_DESTROYED;



/** Virtual empty destructor. */
BlackBoard::~BlackBoard()
{
}


/** Produce interface name from C++ signature.
 * This extracts the interface name for a mangled signature. It has
 * has been coded with GCC (4) in mind and assumes interfaces to be
 * in the fawkes namespace. It cannot deal with anythin else.
 * @param type type name to strip
 * @return stripped class type, use delete to free it after you are done
 */
std::string
BlackBoard::demangle_fawkes_interface_name(const char *type)
{
  std::string t = type;
  t = t.substr( 8 ); // Hack to remove N6fawkes namespace prefix
  t = t.substr( t.find_first_not_of("0123456789") );
  t = t.substr(0, t.length() - 1); // Hack to remove trailing letter
  return t;
}


} // end namespace fawkes

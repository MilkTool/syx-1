/* 
   Copyright (c) 2007-2008 Luca Bruno

   This file is part of Smalltalk YX.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell   
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER    
   DEALINGS IN THE SOFTWARE.
*/

#include "syx-memory.h"
#include "syx-platform.h"
#include "syx-config.h"
#include "syx-error.h"
#include "syx-interp.h"
#include "syx-types.h"
#include "syx-object.h"

#include <assert.h>

static SyxErrorEntry *_syx_error_entries = NULL;
static SyxErrorType _syx_error_entries_top = 0;

#ifndef HAVE_ERRNO_H
int errno=0;
#endif

#define REGISTER_SIGNAL(class, desc, value) \
  (assert (syx_error_register (desc, syx_globals_at (class)) == value))

/*!
  Initialize the error reporting system.

  This function must be called after the image has been loaded or a new image is built.
  Usually, user programs don't need to call this directly.
*/
void
syx_error_init (void)
{
  _syx_error_entries_top = 0;
  REGISTER_SIGNAL ("VMError", "Interpreter internal fail", SYX_ERROR_INTERP);
  REGISTER_SIGNAL ("NotFound", "Not found", SYX_ERROR_NOT_FOUND);
  REGISTER_SIGNAL ("WrongArgumentCount", "Wrong number of arguments", SYX_ERROR_WRONG_ARGUMENT_COUNT);

  /* System signals */
  REGISTER_SIGNAL ("UserInterrupt", "User interrupt", SYX_ERROR_USER_INTERRUPT);
  REGISTER_SIGNAL ("FloatingPointException", "Floating point exception", SYX_ERROR_FLOATING_POINT_EXCEPTION);
  REGISTER_SIGNAL ("TerminationSignal", "Received TERM signal", SYX_ERROR_TERMINATION);
  REGISTER_SIGNAL ("AbnormalTermination", "Abort requested", SYX_ERROR_ABORT);
}

/*!
  Clear all memory allocated to by the error reporting system.

  This function must be called after all other memory has been released and we're really ready to quit,
  so that any other call won't report errors.
  Usually, user programs don't need to call this directly.
*/
void
syx_error_clear (void)
{
  syx_free (_syx_error_entries);
}

/*!
  Register a kind of error.

  Create a new hook for reporting errors from C to the Smalltalk environment.

  \return a number identifying the king of error for future lookups
*/
SyxErrorType
syx_error_register (syx_symbol name, SyxOop klass)
{
  SyxErrorEntry *entry;
  if (!_syx_error_entries)
    _syx_error_entries = (SyxErrorEntry *)syx_calloc (++_syx_error_entries_top, sizeof (SyxErrorEntry));
  else
    _syx_error_entries = (SyxErrorEntry *)syx_realloc (_syx_error_entries,
                                                       (++_syx_error_entries_top) * sizeof (SyxErrorEntry));

  entry = &_syx_error_entries[_syx_error_entries_top - 1];
  entry->name = name;
  entry->klass = klass;

  return _syx_error_entries_top - 1;
}

/*!
  Lookup for an error.

  \param type the type return from syx_error_register
  \return the entry of the error
*/
SyxErrorEntry *
syx_error_lookup (SyxErrorType type)
{
  if (type >= _syx_error_entries_top)
    return NULL;

  return &_syx_error_entries[type];
}

/*!
  Signal an error in the Smalltalk environment, sending #signal to he requested class.

  \param type the type returned by syx_error_register
  \return TRUE if signal succeeded, otherwise FALSE
*/
syx_bool
syx_signal (SyxErrorType type, SyxOop message)
{
  SyxOop context;
  SyxErrorEntry *entry;
  
  entry = syx_error_lookup (type);
  if (!entry)
    return FALSE;

  if (!syx_system_initialized)
    {
      if (SYX_OBJECT_IS_STRING (message) || SYX_OBJECT_IS_SYMBOL (message))
        syx_error ("%s: %s\n", entry->name, SYX_OBJECT_SYMBOL (message));
      else
        syx_error (entry->name);
    }

  if (SYX_IS_NIL (message))
    context = syx_send_unary_message (entry->klass, "signal");
  else
    context = syx_send_binary_message (entry->klass, "signal:", message);

  /* Create a blocking context into another process for system signals, because it's not related
     to any specific process. */
  if (type >= SYX_ERROR_SYSTEM)
    {
      SyxOop process = syx_process_new ();
      syx_interp_enter_context (process, context);
      syx_process_execute_blocking (process);
    }
  else
    syx_interp_enter_context (syx_processor_active_process, context);

  return TRUE;
}

/*!
  Create an error Context in the Smalltalk environment ready to enter a Process.

  \param type the type returned by syx_error_register
*/
SyxOop
syx_signal_create_context (SyxErrorType type, SyxOop message)
{
  SyxOop context;
  SyxErrorEntry *entry;
  
  entry = syx_error_lookup (type);
  if (!entry)
    return syx_nil;

  if (SYX_IS_NIL (message))
    context = syx_send_unary_message (entry->klass, "signal");
  else
    context = syx_send_binary_message (entry->klass, "signal:", message);

  return context;
}

/*!
  Send receiver>>#doesNotUnderstand: with selector.

  \param receiver
  \param selector
*/
syx_bool
syx_signal_does_not_understand(SyxOop receiver, SyxOop selector)
{
  return syx_interp_enter_context (syx_processor_active_process,
                                   syx_send_binary_message (receiver,
                                                            "doesNotUnderstand:",
                                                            selector));
}

/*!
  Display an error then exits.

  This function will show an error MessageBox on Windows CE
*/
#ifndef WINCE
void
syx_error (syx_symbol fmt, ...)
{
  va_list ap;
  fprintf (stderr, "ERROR: ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, "\n");
  exit (EXIT_FAILURE);
}
#else /* !WINCE */
void
syx_error (syx_symbol fmt, ...)
{
  MessageBox (0, SYX_IFDEF_UNICODE (fmt), "Error", 0);
  exit (EXIT_FAILURE);
}
#endif /* WINCE */

/*! Display a warning message */
void
syx_warning (syx_symbol fmt, ...)
{
  va_list ap;
  fprintf (stderr, "WARNING: ");       
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
}

/*! Display perror message and exit */
void
syx_perror (syx_symbol message)
{
#ifdef HAVE_PERROR
  perror (message);
#else
  fputs (message, stderr);
#endif

  exit (EXIT_FAILURE);
}

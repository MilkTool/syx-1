"
Copyright (c) 2007-2008 Luca Bruno

This file is part of Smalltalk YX.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the 'Software'), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
"

!Exception methodsFor: 'testing'!

isHandled
    ^self handlerContext notNil
! !

!Exception methodsFor: 'private'!

handle
    self return: (handlerContext handlerBlock value: self)
!

handlerContextStartingFrom: aContext
    | ctx |
    ctx := aContext.
    [ ctx notNil ]
	whileTrue: [
	    (ctx handles: self)
		ifTrue: [ ^handlerContext := ctx ].
	    ctx := ctx parent ].
    ^nil
!

handlerContext
    "Finds the context which handles the exception"
    handlerContext
	ifNotNil: [ ^handlerContext ].

    ^self handlerContextStartingFrom: signalerContext parent
!

notHandled
    Transcript showCr: 'Unhandled exception... process is going to be terminated.'.
    self debug.
    Processor activeProcess terminate
! !

!Exception methodsFor: 'signaling'!

isResumable
    "Anwser if the signaler context can be resumed back"
    ^true
!

pass
    "Drop the exception to parent contexts"
    self handlerContextStartingFrom: handlerContext parent.
    self signal
!

resume
    "Resume the context which generated the error"
    self isResumable
	ifTrue: [ Processor swapWith: signalerContext parent ]
!

return: anObject
    "Returns anObject"
    Processor leaveTo: handlerContext parent andAnswer: anObject
!

return
    self return: nil
!

description
    ^'An exception has been raised.'
! !

!Error methodsFor: 'signaling'!

description
    ^'An error occurred during program execution.'
! !

!MessageNotUnderstood class methodsFor: 'instance creation'!

message: aMessage receiver: anObject
    ^self new
	message: aMessage receiver: anObject;
	signal
! !

!MessageNotUnderstood methodsFor: 'signaling'!

message
    ^message
! 

receiver
    ^receiver
!

message: aMessage receiver: anObject
    message := aMessage.
    receiver := anObject.
    self messageText: anObject class printString, ': did not understood ', aMessage printString
! !

!ZeroDivide class methodsFor: 'instance creation'!

signal
    ^self signal: 'Division by zero.'
! !

!SubclassResponsibility class methodsFor: 'instance creation'!

signal
    ^self signal: 'This method is not responsibility of this class. A subclass must implement this method.'
! !

!WrongArgumentCount class methodsFor: 'instance creation'!

signal
    ^self signal: 'Wrong number of arguments.'
! !

!NotFound class methodsFor: 'instance creation'!

what: anObject
    ^self signal: 'Object not found: ', anObject displayString
! !




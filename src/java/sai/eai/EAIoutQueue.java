// copyright (c) 1997,1998 stephen f. white
// Modified for EAI FreeWRL code. John Stewart CRC Canada 1999
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

package sai.eai;
import sai.eai.EAIMessage;

public class EAIoutQueue {
    private EAIMessage	head;
    private EAIMessage	tail;

    public		EAIoutQueue() {
	head = tail = null;
    }

    public synchronized void		enqueue(EAIMessage msg) {
	msg.next = head;
	msg.prev = null;
	if (head == null) {
	    tail = msg;
	} else {
	    head.prev = msg;
	}
	head = msg;
    }

    public synchronized EAIMessage		dequeue() {
	if (tail == null) return null;
	EAIMessage msg = tail;
	tail = tail.prev;
	if (tail == null) {
	    head = null;
	} else {
	    tail.next = null;
	}
	msg.prev = msg.next = null;
	return msg;
    }

    public boolean isEmpty() {
	return head == null;
    }
}

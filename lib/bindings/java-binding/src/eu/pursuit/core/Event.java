/*
 * Copyright (C) 2011  Christos Tsilopoulos, Mobile Multimedia Laboratory, 
 * Athens University of Economics and Business 
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

package eu.pursuit.core;

import java.nio.ByteBuffer;


public class Event {
	public static enum EventType{
		START_PUBLISH(100),
		STOP_PUBLISH(101),
		SCOPE_PUBLISHED(102),
		SCOPE_UNPUBLISHED(103),
		PUBLISHED_DATA(104);
		
		private final int index;
		
		private EventType(int id){
			this.index = id;
		}
		
		public int getIndex(){
			return this.index;
		}
		
		public static EventType getById(int id){
			for (EventType ev : values()) {
				if(id == ev.getIndex()){
					return ev;
				}
			}
			return null;
		}
	}
	
	private EventType type;
	private byte [] id; 
	private ByteBuffer data;
	private int dataLength;
	
	/*memory mappings to native code*/
	EventDeleter deleter = null;
	long event_ptr = 0;
	boolean mappedToNativeBuffer = false;
	private boolean freed = false;
	
	public Event(EventType type, byte[] id, ByteBuffer data, int dataLength) {
		this.type = type;
		this.id = id;
		this.data = data;
		this.dataLength = dataLength;
	}

	public Event(EventType type, byte[] id) {
		this(type, id, null, 0);
	}

	public EventType getType() {
		return type;
	}

	public byte [] getId(){
		if(freed){
			throw new IllegalStateException("this event has been freed");
		}
		
		return id;
	}
	
	public int getDataLength(){
		if(freed){
			throw new IllegalStateException("this event has been freed");
		}
		
		return dataLength;
	}
	
	public byte [] getDataCopy(){
		if(freed){
			throw new IllegalStateException("this event has been freed");
		}
		
		return getData(0, this.dataLength);
	}
	
	public ByteBuffer getNativeBuffer(){
		if(freed){
			throw new IllegalStateException("this event has been freed");
		}
		
		return data;
	}
	
	public byte getByte(int pos){
		if(freed){
			throw new IllegalStateException("this event has been freed");
		}
		return data.get(pos);
	}
	
	public void writeTo(ByteBuffer buff){
		if(freed){
			throw new IllegalStateException("this event has been freed");
		}
		
		buff.put(data);
		data.position(0);
	}
		
	public void setNativeMemoryMappings(EventDeleter deleter, long ev){
		//check state
		if(mappedToNativeBuffer){
			throw new IllegalStateException("cannot set native memory mappings more than once. Mappings are already set");
		}
		
		//check input parameters
		if(deleter == null ){
			throw new NullPointerException("deleter is null"); 
		}		
				
		//update state
		this.deleter = deleter;
		event_ptr = ev;
		mappedToNativeBuffer = true;
	}
	
	public void freeNativeBuffer(){
		if(freed){
			return;
		}
					
		if(mappedToNativeBuffer){
			freed  = true;
			deleter.deleteEvent(event_ptr);						
			id = new byte[0]; 
			data = null;
			dataLength = 0;
		}		
	}

	public byte[] getData(int offset, int length) {
		if(freed){
			throw new IllegalStateException("this event has been freed");
		}
		
		byte [] retval = new byte[length];
		this.data.position(offset);
		this.data.get(retval);
		this.data.position(0);
		return retval;
	}	
	
	@Override
	protected void finalize() throws Throwable {
		freeNativeBuffer();
		super.finalize();
	}

}

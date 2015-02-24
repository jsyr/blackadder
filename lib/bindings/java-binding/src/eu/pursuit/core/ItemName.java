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
import java.util.ArrayList;
import java.util.List;

import org.apache.commons.codec.binary.Hex;

public class ItemName {
	private final ScopeID scopeId;
	private final ByteIdentifier rendezvousId;
	
	public ItemName(ScopeID scopeId, ByteIdentifier rendezvousId) {
		super();
		this.scopeId = scopeId;
		this.rendezvousId = rendezvousId;
	}

	public ScopeID getScopeId() {
		return scopeId;
	}

	public ByteIdentifier getRendezvousId() {
		return rendezvousId;
	}
	
	public ItemName deepCopy() {
		return new ItemName(scopeId.deepCopy(), rendezvousId.deepCopy());
	}

	public byte[] toByteArray() {
		int length = 0;
		if(scopeId != null){
			length+=scopeId.getByteLength();
		}
		
		if(rendezvousId!=null){
			length += rendezvousId.getId().length;			
		}
		
		ByteBuffer buffer = ByteBuffer.allocate(length);
		if(scopeId != null){
			scopeId.fill(buffer);
		}
		
		if(rendezvousId!=null){
			buffer.put(rendezvousId.getId());			
		}
		return buffer.array();
	}
	
	@Override
	public String toString(){
		return scopeId.toString() + "/" +Hex.encodeHexString(rendezvousId.getId());
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result
				+ ((rendezvousId == null) ? 0 : rendezvousId.hashCode());
		result = prime * result + ((scopeId == null) ? 0 : scopeId.hashCode());
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		ItemName other = (ItemName) obj;
		if (rendezvousId == null) {
			if (other.rendezvousId != null)
				return false;
		} else if (!rendezvousId.equals(other.rendezvousId))
			return false;
		if (scopeId == null) {
			if (other.scopeId != null)
				return false;
		} else if (!scopeId.equals(other.scopeId))
			return false;
		return true;
	}

	public static ItemName parseSerializedName(byte[] array, int segmentSize) {
		if(array.length % segmentSize != 0){			
			throw new IllegalArgumentException("array length ("+array.length+") not a multiple of segmentSize ("+segmentSize+")");
		}
		
		ByteBuffer bbuffer = ByteBuffer.wrap(array);
		List<ByteIdentifier> list = new ArrayList<ByteIdentifier>();
		int howmany = array.length / segmentSize;
		for (int i = 0; i < howmany-1; i++) {
			byte [] arr = new byte[segmentSize];
			bbuffer.get(arr);
			list.add(new ByteIdentifier(arr));			
		}
		ScopeID scope = new ScopeID(list);
		
		byte [] arr = new byte[segmentSize];
		bbuffer.get(arr);
		ByteIdentifier rid = new ByteIdentifier(arr);
		
		return new ItemName(scope, rid);		
	}

	
}

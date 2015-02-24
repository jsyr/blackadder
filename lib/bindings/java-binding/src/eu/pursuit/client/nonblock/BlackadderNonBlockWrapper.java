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

package eu.pursuit.client.nonblock;

import java.nio.ByteBuffer;
import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicBoolean;

import eu.pursuit.core.BlackadderLoader;
import eu.pursuit.core.Event;
import eu.pursuit.core.Event.EventType;
import eu.pursuit.core.EventDeleter;

public class BlackadderNonBlockWrapper implements EventDeleter {

	private static final int STOP_SINGAL = 255;
	private static boolean USERSPACE = true;
	private final long baPtr;
	private boolean closed = false;
	private EventHandler eventHandler = null;
	private AtomicBoolean ended = new AtomicBoolean(false);

	private Semaphore shutDownSemaphore = new Semaphore(1);
	private Semaphore nativeWorkerThreadDetached = new Semaphore(1);

	private static BlackadderNonBlockWrapper instance = new BlackadderNonBlockWrapper(
			USERSPACE);

	public static void setUserSpace(boolean userspace) {
		USERSPACE = userspace;
	}

	public static BlackadderNonBlockWrapper getWrapper() {
		return instance;
	}

	private BlackadderNonBlockWrapper(boolean userspace) {
		BlackadderLoader.loadSo();

		int user = userspace ? 1 : 0;
		baPtr = create_new_ba(user);
		shutDownSemaphore.drainPermits();
		nativeWorkerThreadDetached.drainPermits();
	}

	private synchronized void setClosed(boolean v) {
		closed = true;
	}

	private synchronized boolean isClosed() {
		return closed;
	}

	public synchronized void setEventHandler(EventHandler handler) {
		this.eventHandler = handler;
	}

	public synchronized void resetHandler() {
		this.eventHandler = null;
	}
	
	protected void detachNativeThead(){
		nativeWorkerThreadDetached.release();
		System.out.println("jni: native thread ready to detach");
	}

	public void publishScope(byte[] scope, byte[] prefixScope, byte strat,
			byte[] strategyOptions) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_publish_scope(baPtr, scope, prefixScope, strat, strategyOptions);
		}
	}

	public void publishItem(byte[] scope, byte[] prefixScope, byte strat,
			byte[] strategyOptions) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_publish_item(baPtr, scope, prefixScope, strat, strategyOptions);
		}
	}

	public void unpublishScope(byte[] scope, byte[] prefixScope, byte strat,
			byte[] strategyOptions) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_unpublish_scope(baPtr, scope, prefixScope, strat, strategyOptions);
		}
	}

	public void unpublishItem(byte[] scope, byte[] prefixScope, byte strat,
			byte[] strategyOptions) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_unpublish_item(baPtr, scope, prefixScope, strat, strategyOptions);
		}
	}

	public void subscribeScope(byte[] scope, byte[] prefixScope, byte strat,
			byte[] strategyOptions) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_subscribe_scope(baPtr, scope, prefixScope, strat, strategyOptions);
		}
	}

	public void subscribeItem(byte[] scope, byte[] prefixScope, byte strat,
			byte[] strategyOptions) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_subscribe_item(baPtr, scope, prefixScope, strat, strategyOptions);
		}
	}

	public void unsubscribeScope(byte[] scope, byte[] prefixScope, byte strat,
			byte[] strategyOptions) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_unsubscribe_scope(baPtr, scope, prefixScope, strat,
					strategyOptions);
		}
	}

	public void unsubscribeItem(byte[] scope, byte[] prefixScope, byte strat,
			byte[] strategyOptions) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_unsubscribe_item(baPtr, scope, prefixScope, strat,
					strategyOptions);
		}
	}

	public void publishData(byte[] scope, byte strat, byte[] strategyOptions,
			byte[] jData) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_publish_data(baPtr, scope, strat, strategyOptions, jData,
					jData.length);
		}
	}

	public void publishData(byte[] scope, byte strat, byte[] strategyOptions,
			ByteBuffer buffer) {
		if (isClosed()) {
			System.err.println("nb_wrapper is closed, message ignored");
		} else {
			c_publish_data_direct(baPtr, scope, strat, strategyOptions, buffer,
					buffer.capacity());
		}
	}

	public void deleteEvent(long event_ptr) {
		c_delete_event(event_ptr);
	}

	private void onEventArrived(long ev_ptr, byte[] id, byte ev_type,
			ByteBuffer data) {

		if (eventHandler == null) {
			int ev_type_int = ev_type & 0x000000FF;
			deleteEvent(ev_ptr);
			if (ev_type_int == STOP_SINGAL) {
				ended.getAndSet(true);
				
				setEventHandler(new EventHandler() {
					
					@Override
					public void handleStopEvent() {}					
					@Override
					public void handleEvent(Event e) {						
					}
				});
				
				startShutDownThread(true);
			}
		} else {
			int ev_type_int = ev_type & 0x000000FF;
			if (ev_type_int == STOP_SINGAL) {
				ended.getAndSet(true);
				deleteEvent(ev_ptr);
				startShutDownThread(false);
			} else {
				EventType type = EventType.getById(ev_type);
				Event event = null;
				if (data.capacity() > 0) {
					event = new Event(type, id, data, data.capacity());
				} else {
					event = new Event(type, id);
				}
				event.setNativeMemoryMappings(this, ev_ptr);
				this.eventHandler.handleEvent(event);
			}
		}
	}

	/**
	 * this is a hack: onEventArrived must IMMEDIATELY return program control to
	 * jni->blackadder library so that the worker thread is detached from the
	 * jvm and blackadder terminates gracefully. Therefore the call to
	 * handleStopEvent will be passed to a separate java thread.
	 */
	private void startShutDownThread(final boolean exit) {

		Thread th = new Thread(new Runnable() {

			@Override
			public void run() {
				
				// wait until native thread detached
				nativeWorkerThreadDetached.acquireUninterruptibly();
								
				//stop any calls to blackadder
				setClosed(true);
				
				//terminate blackadder instance
				close_and_delete(baPtr);
				
				//send the stop event to application
				eventHandler.handleStopEvent();
				
				//inform close if blocked
				shutDownSemaphore.release();
				if(exit){
					System.err.println("system will now exit with return code 1");
					System.exit(1);
				}
			}
		});
		th.setName("finalizerThread");
		th.start();
	}

	/**
	 * Sends the end signal to Blackadder library and <strong>blocks</strong>
	 * until the END_EVENT is returned from the Blackadder library.
	 */
	public void close() {
		if (ended.compareAndSet(false, true)) {
			end(baPtr);
			shutDownSemaphore.acquireUninterruptibly();
		}
	}

	@Override
	protected void finalize() throws Throwable {
		if (!isClosed()) {
			close();
		}
		super.finalize();
	}

	/* native methods */
	private native long create_new_ba(int userspace);

	private native void close_and_delete(long ba_ptr);

	private native void c_publish_scope(long ba_ptr, byte[] scope,
			byte[] prefixScope, byte strat, byte[] strategyOptions);

	private native void c_publish_item(long ba_ptr, byte[] scope,
			byte[] prefixScope, byte strat, byte[] strategyOptions);

	private native void c_unpublish_scope(long ba_ptr, byte[] scope,
			byte[] prefixScope, byte strat, byte[] strategyOptions);

	private native void c_unpublish_item(long ba_ptr, byte[] scope,
			byte[] prefixScope, byte strat, byte[] strategyOptions);

	private native void c_subscribe_scope(long ba_ptr, byte[] scope,
			byte[] prefixScope, byte strat, byte[] strategyOptions);

	private native void c_subscribe_item(long ba_ptr, byte[] scope,
			byte[] prefixScope, byte strat, byte[] strategyOptions);

	private native void c_unsubscribe_scope(long ba_ptr, byte[] scope,
			byte[] prefixScope, byte strat, byte[] strategyOptions);

	private native void c_unsubscribe_item(long ba_ptr, byte[] scope,
			byte[] prefixScope, byte strat, byte[] strategyOptions);

	private native void c_publish_data(long ba_ptr, byte[] scope, byte strat,
			byte[] strategyOptions, byte[] dataBuffer, int length);

	private native void c_publish_data_direct(long ba_ptr, byte[] scope,
			byte strat, byte[] strategyOptions, ByteBuffer buffer, int length);

	private native void c_delete_event(long event_ptr);

	private native void end(long baPtr);
}

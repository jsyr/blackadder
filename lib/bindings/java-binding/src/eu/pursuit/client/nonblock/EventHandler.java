package eu.pursuit.client.nonblock;

import eu.pursuit.core.Event;


public interface EventHandler {

	void handleEvent(Event e);

	void handleStopEvent();
}

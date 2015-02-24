/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
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

#ifndef CLICK_INTRADOMAINRENDEZVOUS_HH
#define CLICK_INTRADOMAINRENDEZVOUS_HH

#include <click/config.h>

#include "rendezvous_interface.hh"

CLICK_DECLS

class IntraDomainRendezvous : public RendezvousInterface {
public:
    IntraDomainRendezvous(RV *_rv_element);
    ~IntraDomainRendezvous();
    unsigned int handleRVRequest(unsigned char type, String &nodeID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief Lists all idenfifiers of all scopes and information items. */
    String listInfoStructs();
private:
    /**@brief this method is called if the type of request is PUBLISH_SCOPE.
     * 
     * If ID is a single fragment and prefixID the zero string then the request is about publishing a root scope (see publish_root_scope() method).
     * 
     * If prefixID contains one or more fragments (PURSUIT_ID_LEN each) and ID is a single fragment the the request is about publishing an inner scope (see publish_inner_scope() method).
     * 
     * If both ID and prefixID contain multiple fragments (PURSUIT_ID_LEN each) then the request is about republishing an existing scope under another scope (see republish_inner_scope() method).
     * 
     * @param _publisher the RemoteHost that issued the request (it can be the local host as well by using the node label of this node).
     * @param ID A Click's String that is the identifier. It can be either a single fragment identifier (PURSUIT_ID_LEN) or multiple fragments (PURSUIT_ID_LEN each)
     * @param prefixID A Click's String that is the prefix identifier. It can be a zero string, a single fragment identifier (PURSUIT_ID_LEN) or multiple fragments (PURSUIT_ID_LEN each)
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int publish_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief This method publishes a root scope in the information graph maintained by this rendezvous element.
     * 
     * Publishing a root Scope is rather straightforward. If the Scope does not exist it is created and added to the index maintained by the RV (the father scope is NULL).
     * If the _publisher wasn't in the set of publishers for this root scope, the set of publishers is updated and the _publisher (RemoteHost) is updated with the published root Scope.
     * 
     * @note There is no need for rendezvous to take place. If the root scope existed and strategy does not match with the previously assigned dissemination strategy, the request is rejected.
     * 
     * @param _publisher the RemoteHost issued this request.
     * @param ID the identifier of the root scope (a single fragment of PURSUIT_ID_LEN size).
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int publish_root_scope(String &publisherID, String &ID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief This method publishes an inner scope in the information graph maintained by this rendezvous element.
     * 
     * The full identifier of the inner scope will be fullID = prefixID + ID. If an InformationItem with this identifier already exists, the method returns a respective error code.
     * 
     * If the father scope (identified by prefixID) does not exist the method returns a respective error code.
     * 
     * If the new scope already exists the set of publishers is updated with the _publisher to which the new scope is assigned (if strategy matches the dissemination strategy of the existing scope).
     * 
     * In the opposite case, the scope is created if strategy matches the dissemination strategy of the father Scope. The _publisher is stored in the Scope's set of publishers and the new Scope is assigned to the _publisher.
     * After creating the inner scope, the RV finds and notifies all subscribers of the father Scope (only a single father Scope can exist in this case) about this (see notifySubscribers()).
     * 
     * @note: For all operations described for the RV only subscribers of the father scope are notified and NOT subscribers of all ancestor scopes.
     * 
     * @param _publisher the RemoteHost issued this request.
     * @param ID a single fragment identifier (PURSUIT_ID_LEN) identifying the new Scope in the context of the existing Scope.
     * @param prefixID the full identifier of the exisiting Scope under which the Scope will be published.
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int publish_inner_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief This method republishes an inner scope under an existing Scope in the information graph maintained by this rendezvous element.
     * 
     * The full identifier of the republished scope will be fullID = prefixID + suffixID, where suffixID = ID.substring(ID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN).
     * If an InformationItem with this identifier already exists, the method returns a respective error code.
     * 
     * If the father scope (identified by prefixID) does not exist the method returns a respective error code.
     * 
     * If the Scope to be republished does not exist the method returns a respective error code.
     * 
     * If the Scope has been previously republished and the strategy assigned in this request matches, the _publisher will be added to the previously republished Scope which will be assigned to the _publisher.
     * 
     * In the opposite case, the scope is republished if strategy matches the dissemination strategy of the father Scope. The _publisher is stored in the Scope's set of publishers and the republished Scope is assigned to the _publisher.
     * 
     * If the scope was not previously republished and this operation succeeds, subscribers of the father Scope need to be notified. 
     * Note that a slightly variation of the notifySubscribers() method is used, so that subscribers of the father scope of the originally existed scope are not notified (an extra ID is passed by reference).
     * 
     * @note: For all operations described for the RV only subscribers of the father scope are notified and NOT subscribers of all ancestor scopes.
     * 
     * @param _publisher the RemoteHost issued this request.
     * @param ID a single fragment identifier (PURSUIT_ID_LEN) identifying the new Scope in the context of the existing Scope.
     * @param prefixID the full identifier of the existing Scope under which the Scope will be published.
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int republish_inner_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief this method is called if the type of request is PUBLISH_INFO.
     * 
     * If prefixID contains one or more fragments (PURSUIT_ID_LEN each) and ID is a single fragment the the request is about publishing an information item under a scope (see advertise_info() method).
     * 
     * If both ID and prefixID contain multiple fragments (PURSUIT_ID_LEN each) then the request is about republishing an existing information item under another scope (see readvertise_info() method).
     * 
     * @param _publisher the RemoteHost that issued the request (it can be the local host as well by using the node label of this node).
     * @param ID A Click's String that is the identifier. It can be either a single fragment identifier (PURSUIT_ID_LEN) or multiple fragments (PURSUIT_ID_LEN each)
     * @param prefixID A Click's String that is the prefix identifier. It can be a zero string, a single fragment identifier (PURSUIT_ID_LEN) or multiple fragments (PURSUIT_ID_LEN each)
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int publish_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief This method publishes an InformationItem in the information graph maintained by this rendezvous element.
     * 
     * The full identifier of the InformationItem will be fullID = prefixID + ID. If a Scope with this identifier already exists, the method returns a respective error code.
     * 
     * If the father scope (identified by prefixID) does not exist the method returns a respective error code.
     * 
     * If the InformationItem already exists the set of publishers is updated with the _publisher to which the InformationItem is assigned (if strategy matches the dissemination strategy of the existing InformationItem).
     * 
     * In the opposite case, the InformationItem is created if strategy matches the dissemination strategy of the father Scope. The _publisher is stored in the InformationItem's set of publishers and the InformationItem is assigned to the _publisher.
     * 
     * In the two latter cases (when the InformationItem is created or when the set of publishers of the existing InformationItem is updated) rendezvous() takes place.
     * For that, the set of subscribers is calculated (by calling getSubscribers() to the InformationItem and to the father Scope) and the rendezvous() method is called.
     * 
     * @param _publisher the RemoteHost issued this request.
     * @param ID a single fragment identifier (PURSUIT_ID_LEN) identifying the new InformationItem in the context of the existing Scope.
     * @param prefixID the full identifier of the exisiting Scope under which the InformationItem will be published.
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int advertise_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief This method republishes an InformationItem under an existing Scope in the information graph maintained by this rendezvous element.
     * 
     * The full identifier of the republished InformationItem will be fullID = prefixID + suffixID, where suffixID = ID.substring(ID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN).
     * If a Scope with this identifier already exists, the method returns a respective error code.
     * 
     * If the father scope (identified by prefixID) does not exist the method returns a respective error code.
     * 
     * If the InformationItem to be republished does not exist the method returns a respective error code.
     * 
     * If the InformationItem has been previously republished and the strategy assigned in this request matches, the _publisher will be added to the previously republished InformationItem which will be assigned to the _publisher.
     * 
     * In the opposite case, the InformationItem is republished if strategy matches the dissemination strategy of the father Scope. The _publisher is stored in the InformationItem's set of publishers and the republished InformationItem is assigned to the _publisher.
     * 
     * In the two latter cases (when the InformationItem is republished or when the set of publishers of the already republished InformationItem is updated) rendezvous() takes place.
     * For that, the set of subscribers is calculated (by calling getSubscribers() to the InformationItem and to ALL father Scopes) and the rendezvous() method is called.
     * 
     * @param _publisher the RemoteHost issued this request.
     * @param ID a single fragment identifier (PURSUIT_ID_LEN) identifying the new Scope in the context of the existing Scope.
     * @param prefixID the full identifier of the existing Scope under which the Scope will be published.
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int readvertise_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief This method will try to unpublish the Scope identified by the fullID = prefixID + ID.
     * 
     * This method is quite complex and the result may vary.
     * If the Scope does not exist or the provided strategy does not match with the existing Scope's strategy the method returns with a respective error code.
     * 
     * <b>If the Scope is a root Scope then:</b>
     * 
     *          this method will try to unpublish all InformationItems residing under that Scope (see unpublish_info()).
     * 
     *          _publisher is removed from the Scope's publishers and the Scope is removed from _publisher's Scopes.
     * 
     *          If there are other any subScopes or InformationItems under this Scope, then the Scope cannot be deleted. 
     * 
     *          Else the Scope is deleted and all references to it are deleted ONLY if there are no other publishers or subscribers for that Scope.
     * 
     * <b>If not then:</b>
     * 
     *          this method will try to unpublish all InformationItems residing under that Scope (see unpublish_info()).
     * 
     *          The _publisher is deleted from the right branch of the graph that identifies the Scope (in case the Scope has multiple fathers) and the respective information identifier is deleted from the Scopes of the _publisher..
     * 
     *          If there are other any subScopes or InformationItems under this Scope, then the Scope cannot be deleted. 
     * 
     *          Else the Scope is deleted (or just a single branch in the information graph) and all references to it are deleted ONLY if there are no other publishers or subscribers for that Scope.
     * 
     * @note In any case _publisher will be removed from this Scope (if it previously existed).
     * @param _publisher the RemoteHost issued this request.
     * @param ID a single fragment identifier (PURSUIT_ID_LEN) identifying the Scope to be deleted in the context of an existing Scope.
     * @param prefixID the full identifier of the father Scope (it can be NULL when deleting root Scopes).
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int unpublish_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief This method will try to unpublish the InformationItem identified by the fullID = prefixID + ID.
     * 
     * If the InformationItem does not exist or the provided strategy does not match with the existing InformationItem's strategy the method returns with a respective error code.
     * 
     * The _publisher is then removed from the set of publishers for this InformationItem, and the item is removed from the _publisher's published items.
     * 
     * If there are no other publishers or subscribers for that InformationItem, the item is deleted from the graph (or only one branch if is published under multiple scopes) and all references to it are deleted (e.g. from father scope).
     * @note It is important to understand that when the _publisher was deleted from the InformationItem and the item is not deleted, rendezvous() must take place again with the rest of publishers (if any).
     * 
     * @param _publisher the RemoteHost issued this request.
     * @param ID a single fragment identifier (PURSUIT_ID_LEN) identifying the InformationItem to be deleted in the context of an existing Scope.
     * @param prefixID the full identifier of the father Scope (it can be NULL when deleting root Scopes).
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int unpublish_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief this method will subscribe _subscriber to the Scope identified by the fullID = prefixID + ID.
     * 
     * If prefixID is a single fragment then it calls subscribe_root_scope().
     * If prefixID consists of multiple fragments then it calls subscribe_inner_scope().
     * 
     * @param _subscriber the RemoteHost that issued the request (it can be the local host as well, by using the node label of this node).
     * @param ID A Click's String that is the identifier of the Scope in the context of its father Scope. It has to be a single fragment (PURSUIT_ID_LEN).
     * @param prefixID A Click's String that is the prefix identifier. It can be a zero string, a single fragment identifier (PURSUIT_ID_LEN) or it can consist of multiple fragments (PURSUIT_ID_LEN each)
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int subscribe_scope(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief this method will subscribe _subscriber to the root Scope identified by ID.
     * 
     * If the root Scope does not exist, it is created and the subscription is added. _subscriber scope set is also updated.
     * There is no need for rendezvous since the scope has just been created.
     * 
     * If the root Scope existed and the strategy assigned to this request matches, the subscriber is added.
     * Then, the _subscriber is notified about all direct subscopes of the Scope. 
     * Finally, rendezvous takes place for <b>each information item</b> under this Scope. 
     * Note that all publishers and subscribers for all information items must be taken into account when doing rendezvous.
     * 
     * @param _subscriber the RemoteHost that issued the request
     * @param ID a single fragment identifying the root Scope. 
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int subscribe_root_scope(String &subscriberID, String &ID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief this method will subscribe _subscriber to Scope identified by the fullID = prefixID + ID.
     * 
     * As usual, this method will look if strategies do not match, if the father scope does not exists and if a publication with the same fullID exists. In any of these cases it will return one of the respective error codes.
     * 
     * If the Scope with fullID does not exist the method will create and add the subscription of _subscriber. Note that since a Scope is created potential subscribers of the father scope must be notified.
     * If the Scope exists the subscription is added.
     * Then, the _subscriber is notified about all direct subscopes of the Scope. 
     * Finally, rendezvous takes place for <b>each information item</b> under this Scope. 
     * Note that all publishers and subscribers for all information items must be taken into account when doing rendezvous.
     * 
     * @param _subscriber the RemoteHost that issued the request
     * @param ID a single fragment identifying the Scope in the context of the father Scope. 
     * @param prefixID one or more fragments identifying the father Scope.
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int subscribe_inner_scope(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief this method will subscribe _subscriber to InformationItem identified by the fullID = prefixID + ID.
     * 
     * As usual, this method will look if strategies do not match, if the father scope does not exists and if a scope with the same fullID exists. In any of these cases it will return one of the respective error codes.
     * If the InformationItem with fullID does not exist the method will create and add the subscription of _subscriber.
     * If the InformationItem exists the subscription is added and rendezvous takes place for this specific InformationItem. 
     * Note that all publishers and subscribers for this information items must be taken into account when doing rendezvous.
     * 
     * @param _subscriber the RemoteHost that issued the request
     * @param ID a single fragment identifying the InformationItem in the context of the father Scope. 
     * @param prefixID one or more fragments identifying the father Scope.
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int subscribe_info(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief This method will unsubscribe _subscriber from Scope identified by the fullID = prefixID + ID.
     * The Scope may be deleted if there are no other publishers and subscribers as well as other subscopes and information items under it.
     * Rendezvous must happen for all information items under the Scope
     * 
     * @param _subscriber the RemoteHost that issued the request
     * @param ID a single fragment identifying the Scope in the context of the father Scope. 
     * @param prefixID one or more fragments identifying the father Scope.
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int unsubscribe_scope(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief This method will unsubscribe _subscriber from InformationItem identified by the fullID = prefixID + ID.
     * The item may be deleted if there are no other publishers and subscribers.
     * Rendezvous must happen for this information item if more subscribers and publishers exist.
     * 
     * @param _subscriber the RemoteHost that issued the request
     * @param ID a single fragment identifying the InformationItem in the context of the father Scope. 
     * @param prefixID one or more fragments identifying the father Scope.
     * @param strategy the dissemination strategy assigned to the request.
     * @return one of the return codes (see helper.hh).
     */
    unsigned int unsubscribe_info(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len);
    /**@brief It looks into the RV::pub_sub_Index for the RemoteHost identified by the node label nodeID (a Blackadder node).
     * 
     * @param nodeID the identifier of the Blackadder node (it can be the label of this Blackadder node). See LocalHost for details.
     * @return an existing RemoteHost or a constructed object.
     */
    RemoteHost *getRemoteHost(String & nodeID);
    /**@brief This method will notify all subscribers in the subscribers set about the existence of Scope sc.
     * 
     * Depending on the dissemination strategy it will may need to request the Topology Manager to notify subscribers on behalf of this RV element.
     * 
     */
    void notifySubscribers(Scope *sc, StringSet &IDs, unsigned char notification_type, RemoteHostSet &_subscribers);
    /**@brief This the rendezvous method that is run when several pub/sub requests are issued.
     * 
     * It finds all publishers for the provided InformationItem and "matches" them with the subscribers. This match is dissemination strategy specific.
     * 
     * Depending on the dissemination strategy the Dispatcher may be notified to START or STOP_PUBLISH data for the InformationItem.
     * The topology manager may be required in order to create multicast trees and the respective LIPSIN identifiers. In such case the Top[ology Manager will notify the publishers directly.
     * 
     * @param pub a pointer to an InformationItem for which rendezvous takes place.
     * @param _subscribers a reference to a set of subscribers for which rendezvous will happen for the provided InformationItem.
     */
    void rendezvous(InformationItem *pub, RemoteHostSet &_publishers, RemoteHostSet &_subscribers);
    /**@brief Using the Blackadder API this method will publish a request (using the IMPLICIT_RENDEZVOUS strategy) for topology formation to the topology manager.
     * 
     * This publication will contain the type of this request which is MATCH_PUB_SUBS, the set of labels for publishers and subscribers and all Information Identifiers.
     * 
     * Information IDs are not required by the TM, but the TM will then use them to notify all publishers to START or STOP publishing data for the InformationItem.
     * 
     * @param pub a pointer to an InformationItem for which rendezvous took place.
     * @param _publishers a reference to a set of publishers for which rendezvous took place for the provided InformationItem.
     * @param _subscribers a reference to a set of subscribers for which rendezvous took place for the provided InformationItem.
     * @param IDs the set of identifiers identifying the InformationItem.
     */
    void requestTopologyFormationForPublishers(InformationItem *pub, RemoteHostSet &_publishers, RemoteHostSet &_subscribers);
    /**@brief Using the Blackadder API this method will publish a request (using the IMPLICIT_RENDEZVOUS strategy) for topology formation so that the Topology Manager will notify subscribers about a new scope.
     * 
     * @param request_type currently unused BUT BERY SOON THIS HAS TO BE SOMETHING LIKE NEW_SCOPE and DELETED_SCOPE
     * @param _subscribers a reference to a set of subscribers that must be notified
     * @param IDs the set of identifiers identifying the Scope.
     */
    void requestTopologyFormationForSubscribers(Scope *sc, StringSet &IDs, unsigned char notification_type, RemoteHostSet &subscribers);
    /**@brief A HashTable that maps information identifiers to pointers to Scope.
     * 
     * Multiple identifiers may be mapped to the same Scope since multiple paths from the information graph may lead to the same scope.
     */
    ScopeHashMap scopeIndex;
    /**@brief A HashTable that maps information identifiers to pointers to InformationItem.
     * 
     * Multiple identifiers may be mapped to the same InformationItem since multiple paths from the information graph may lead to the same InformationItem.
     */
    IIHashMap pubIndex;
    /**@brief A Click's HashTable that maps node labels to pointers to RemoteHost.
     * 
     * These labels correspond to Blackadder nodes, one of them being the Dispatcher.
     */
    RemoteHostHashMap pub_sub_Index;
    String RVScope;
};

CLICK_ENDDECLS
#endif

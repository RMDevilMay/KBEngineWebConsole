void Machine::stopserver(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	int32 uid = 0;
	COMPONENT_TYPE componentType;
	COMPONENT_ID componentID;
	bool success = true;

	uint16 finderRecvPort = 0;

	s >> uid;
	s >> componentType;
	
	// 如果组件ID大于0则仅停止指定ID的组件
	s >> componentID;
	
	if(s.length() > 0)
	{
		s >> finderRecvPort;
	}

	INFO_MSG(fmt::format("Machine::stopserver: request uid={}, componentType={}, componentID={},  addr={}\n", 
		uid,  COMPONENT_NAME_EX(componentType), componentID, pChannel->c_str()));

	if(ComponentName2ComponentType(COMPONENT_NAME_EX(componentType)) == UNKNOWN_COMPONENT_TYPE)
	{
		ERROR_MSG(fmt::format("Machine::stopserver: component({}) error!", 
			(int)ComponentName2ComponentType(COMPONENT_NAME_EX(componentType))));

		return;
	}

	Components::COMPONENTS& components = Components::getSingleton().getComponents(componentType);

	if(components.size() > 0)
	{
		Components::COMPONENTS::iterator iter = components.begin();

		for(; iter != components.end(); )
		{
			Components::ComponentInfos* cinfos = &(*iter);

			if(componentID > 0 && componentID != cinfos->cid)
			{
				iter++;
				continue;
			}

			if(cinfos->uid != uid)
			{
				iter++;
				continue;
			}

			if(componentType != cinfos->componentType)
			{
				iter++;
				continue;
			}

			if(((*iter).flags & COMPONENT_FLAG_SHUTTINGDOWN) > 0)
			{
				iter++;
				continue;
			}

			INFO_MSG(fmt::format("--> stop {}({}), addr={}\n", 
				(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? cinfos->pIntAddr->c_str() : "unknown")));

			bool usable = checkComponentUsable(&(*iter), false, false);
		
			if(!usable)
			{
				iter = components.erase(iter);
				continue;
			}


			Network::Bundle closebundle;
			if(componentType != BOTS_TYPE)
			{
				COMMON_NETWORK_MESSAGE(componentType, closebundle, reqCloseServer);
			}
			else
			{
				closebundle.newMessage(BotsInterface::reqCloseServer);
			}

			Network::EndPoint ep1;
			ep1.socket(SOCK_STREAM);

			if (!ep1.good())
			{
				ERROR_MSG("Machine::stopserver: Failed to create socket.\n");
				success = false;
				break;
			}
		
			if(ep1.connect((*iter).pIntAddr.get()->port, (*iter).pIntAddr.get()->ip) == -1)
			{
				ERROR_MSG(fmt::format("Machine::stopserver: connect server error({})!\n", kbe_strerror()));
				success = false;
				break;
			}

			ep1.setnonblocking(false);
			ep1.send(&closebundle);

			Network::TCPPacket recvpacket;
			recvpacket.resize(255);

			fd_set	fds;
			struct timeval tv = { 0, 1000000 }; // 1000ms

			FD_ZERO( &fds );
			FD_SET((int)ep1, &fds);

			int selgot = select(ep1+1, &fds, NULL, NULL, &tv);
			if(selgot == 0)
			{
				// 超时, 可能对方繁忙
				ERROR_MSG(fmt::format("--> stop {}({}), addr={}, timeout!\n", 
					(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? 
					cinfos->pIntAddr->c_str() : "unknown")));

				iter++;
				continue;
			}
			else if(selgot == -1)
			{
				WARNING_MSG(fmt::format("--> stop {}({}), addr={}, recv_len == -1!\n", 
					(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? 
					cinfos->pIntAddr->c_str() : "unknown")));

				iter++;
				continue;
			}

			(*iter).flags |= COMPONENT_FLAG_SHUTTINGDOWN;

			int len = ep1.recv(recvpacket.data(), 1);
			if(len != 1)
			{
				ERROR_MSG(fmt::format("--> stop {}({}), addr={}, recv_len != 1!\n", 
					(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? 
					cinfos->pIntAddr->c_str() : "unknown")));

				success = false;
				break;
			}

			recvpacket >> success;
			iter++;
		}
	}
	else
	{
		INFO_MSG(fmt::format("Machine::stopserver: uid={}, {} size is 0, addr={}\n", 
			uid,  COMPONENT_NAME_EX(componentType), pChannel->c_str()));
	}

	Network::Bundle* pBundle = Network::Bundle::createPoolObject();
	(*pBundle) << success;

	if(finderRecvPort != 0)
	{
		Network::EndPoint ep;
		ep.socket(SOCK_DGRAM);

		if (!ep.good())
		{
			ERROR_MSG("Machine::stopserver: Failed to create socket.\n");
			Network::Bundle::reclaimPoolObject(pBundle);
			return;
		}
	
		ep.sendto(pBundle, finderRecvPort, pChannel->addr().ip);
		Network::Bundle::reclaimPoolObject(pBundle);
	}
	else
	{
		pChannel->send(pBundle);
	}
}
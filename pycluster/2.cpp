void Machine::killserver(Network::Channel* pChannel, KBEngine::MemoryStream& s)
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

	if (s.length() > 0)
	{
		s >> finderRecvPort;
	}

	INFO_MSG(fmt::format("Machine::killserver: request uid={}, componentType={}, componentID={},  addr={}\n",
		uid, COMPONENT_NAME_EX(componentType), componentID, pChannel->c_str()));

	if (ComponentName2ComponentType(COMPONENT_NAME_EX(componentType)) == UNKNOWN_COMPONENT_TYPE)
	{
		ERROR_MSG(fmt::format("Machine::killserver: component({}) error!",
			(int)ComponentName2ComponentType(COMPONENT_NAME_EX(componentType))));

		return;
	}

	Components::COMPONENTS& components = Components::getSingleton().getComponents(componentType);

	if (components.size() > 0)
	{
		Components::COMPONENTS::iterator iter = components.begin();

		for (; iter != components.end();)
		{
			Components::ComponentInfos* cinfos = &(*iter);

			if (componentID > 0 && componentID != cinfos->cid)
			{
				iter++;
				continue;
			}

			if (cinfos->uid != uid)
			{
				iter++;
				continue;
			}

			if (componentType != cinfos->componentType)
			{
				iter++;
				continue;
			}

			INFO_MSG(fmt::format("--> kill {}({}), addr={}\n",
				(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? cinfos->pIntAddr->c_str() : "unknown")));

			int killtry = 0;
			bool killed = false;

			while (killtry++ < 10)
			{
				// 杀死进程
				std::string killcmd;

#if KBE_PLATFORM == PLATFORM_WIN32
				killcmd = fmt::format("taskkill /f /t /pid {}", cinfos->pid);
				
#else
				killcmd = fmt::format("kill -s 9 {}", cinfos->pid);
#endif

				if (-1 == system(killcmd.c_str()))
				{
					ERROR_MSG(fmt::format("Machine::killserver: system({}) error!",
						killcmd));
				}

				bool usable = checkComponentUsable(&(*iter), false, false);

				if (!usable)
				{
					iter = components.erase(iter);
					killed = true;
					break;
				}

				sleep(100);
			}

			if (killed)
				continue;

			success = false;
			break;
		}
	}
	else
	{
		INFO_MSG(fmt::format("Machine::killserver: uid={}, {} size is 0, addr={}\n",
			uid, COMPONENT_NAME_EX(componentType), pChannel->c_str()));
	}

	Network::Bundle* pBundle = Network::Bundle::createPoolObject();
	(*pBundle) << success;

	if (finderRecvPort != 0)
	{
		Network::EndPoint ep;
		ep.socket(SOCK_DGRAM);

		if (!ep.good())
		{
			ERROR_MSG("Machine::killserver: Failed to create socket.\n");
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
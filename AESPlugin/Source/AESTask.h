#ifndef AESTASK_H
#define AESTASK_H

#include "Command.h"
#include "Connection.h"
#include "AESConnection.h"

#include <string>
#include <vector>

class AESCommand;

// AES main task
class AESTask {
public:
    AESTask();
    ~AESTask();
    
    inline void SetURL(const std::string& URL) { m_URL = URL; }
    inline const std::string& GetURL() const { return m_URL; }

    inline void SetUserName(const std::string& userName) { m_UserName = userName; }
    inline const std::string& GetUserName() const { return m_UserName; }

    inline void SetPassword(const std::string& password) { m_Password = password; }
    inline const std::string& GetPassword() const { return m_Password; }

    inline void SetProjectPath(const std::string& path) { m_ProjectPath = path; }
    inline const std::string& GetProjectPath() const { return m_ProjectPath; }
    
    int Run();
    
    inline bool IsConnected() const { return m_IsConnected; }
    
private:
    int Connect();
    void Disconnect();
    
    bool Dispatch(UnityCommand command, const std::vector<std::string>& args);
    
    bool m_IsConnected;
	std::string m_URL;
	std::string m_UserName;
	std::string m_Password;
	std::string m_ProjectPath;

    friend class AESCommand;
    Connection* m_Connection;
    AESConnection* m_AES;
    
	static AESTask* s_Singleton;
};

#endif // AESTASK_H
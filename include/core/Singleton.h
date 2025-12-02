#pragma once
template<class T>
class Singleton
{
public:
	static inline T& GetInstance()
	{
		static T instance;
		return instance;
	}
protected:
	Singleton() = default;
	virtual ~Singleton() = default;

private:
	void operator=(const Singleton& obj) = delete;
	Singleton(const Singleton& obj) = delete;
};

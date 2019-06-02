#pragma once
#include <functional>
#include <vector>
#include <mutex>

namespace evnt
{
	template<class... TArgs>
	class Event;

	template <class... TArgs>
	class Handler
	{
	public:
		using EventT = Event<TArgs...>;
		
		Handler(std::function<void(TArgs ...)> function) noexcept;
		
		/// \brief creates handler from a class member-function
		/// \tparam TClass class type
		/// \tparam TReturn return value of the function will be ignored
		/// \param object class instance
		/// \param func member-function pointer
		template<class TClass, class TReturn>
		Handler(TClass* object, TReturn(TClass::* func)(TArgs...)) noexcept;

		/// \brief creates handler from a class member-function
		/// \tparam TClass class type
		/// \tparam TReturn return value of the function will be ignored
		/// \param object class instance
		/// \param func member-function pointer
		template<class TClass, class TReturn>
		Handler(const TClass* object, TReturn(TClass::* func)(TArgs...) const) noexcept;

		~Handler();
		Handler(const Handler<TArgs...>&) = delete;
		Handler& operator=(const Handler<TArgs...>&) = delete;
		Handler(Handler<TArgs...>&&);
		Handler& operator=(Handler<TArgs...>&&);

		/// \brief calls function that was given to the constructor
		void invoke(TArgs ... args) const;
		/// \brief unsubscribes from all events
		void reset();
		/// \brief swaps event subscriptions and the internal function
		void swap(Handler<TArgs...>& other);
	private:
		void removeEvent(EventT* e);
		void addEvent(EventT* e);

		std::function<void(TArgs ...)> m_function;
		std::vector<EventT*> m_subscribedTo;

		friend EventT;
	};

	template<class... TArgs>
	class Event
	{
	public:
		using HandlerT = Handler<TArgs...>;

		Event() = default;
		Event(const Event<TArgs...>&) = delete;
		Event& operator=(const Event<TArgs...>&) = delete;
		Event(Event<TArgs...>&&);
		Event& operator=(Event<TArgs...>&&);
		~Event();
		/// \brief calls invoke on all event handlers
		void invoke(TArgs... args) const;

		/// \brief adds a new event handler
		void subscribe(HandlerT* handler);
		/// \brief removes the first occurence of this event handler
		void unsubscribe(HandlerT* handler);
		/// \brief removes all handlers
		void reset();
		/// \brief swaps event handlers
		void swap(Event<TArgs...>& other);

		/// \brief creates a new handler and subscribes the handler to this event
		/// \param function function for handler creation
		/// \return the new handler
		HandlerT subscribe(std::function<void(TArgs ...)> function);
	private:
		/// adds handler to m_handler but does not add the event to handler->m_subscribedTo
		void subscribeConstHandler(HandlerT* handler);
		/// removed the handler from m_handler but does not remove the event from handler->m_subscribedTo
		void unsubscribeConstHandler(const HandlerT* handler);

		std::vector<HandlerT*> m_handler;

		friend HandlerT;
	};

	// Handler implementation

	template <class ... TArgs>
	Handler<TArgs...>::Handler(std::function<void(TArgs ...)> function) noexcept:
		m_function(move(function))
	{
	}

	template <class ... TArgs>
	template <class TClass, class TReturn>
	Handler<TArgs...>::Handler(TClass* object, TReturn( TClass::* func)(TArgs...)) noexcept
		:
	m_function([object, func](TArgs... args)
	{
		std::invoke(func, object, args...);
	}){}

	template <class ... TArgs>
	template <class TClass, class TReturn>
	Handler<TArgs...>::Handler(const TClass* object, TReturn( TClass::* func)(TArgs...) const) noexcept
		:
	Handler(const_cast<TClass*>(object), reinterpret_cast<TReturn(TClass::*)(TArgs...)>(func))
	{}

	template <class ... TArgs>
	Handler<TArgs...>::~Handler()
	{
		reset();
	}

	template <class ... TArgs>
	Handler<TArgs...>::Handler(Handler<TArgs...>&& other)
	{
		swap(other);
	}

	template <class ... TArgs>
	Handler<TArgs...>& Handler<TArgs...>::operator=(Handler<TArgs...>&& other)
	{
		swap(other);
		return *this;
	}

	template <class ... TArgs>
	void Handler<TArgs...>::invoke(TArgs ... args) const
	{
		m_function(args...);
	}

	template <class ... TArgs>
	void Handler<TArgs...>::reset()
	{
		// unsubscribe from all events
		for(auto e : m_subscribedTo)
			e->unsubscribeConstHandler(this);

		m_subscribedTo.clear();
	}

	template <class ... TArgs>
	void Handler<TArgs...>::swap(Handler<TArgs...>& other)
	{
		// unsubscribe from old events
		for (auto e : m_subscribedTo)
			e->unsubscribeConstHandler(this);
		for (auto e : other.m_subscribedTo)
			e->unsubscribeConstHandler(&other);

		// swap data
		std::swap(m_subscribedTo, other.m_subscribedTo);
		std::swap(m_function, other.m_function);

		// re-subscribe to events
		for (auto e : m_subscribedTo)
			e->subscribeConstHandler(this);
		for (auto e : other.m_subscribedTo)
			e->subscribeConstHandler(&other);
	}

	template <class ... TArgs>
	void Handler<TArgs...>::removeEvent(EventT* e)
	{
		auto it = std::find(m_subscribedTo.begin(), m_subscribedTo.end(), e);
		if (it != m_subscribedTo.end())
			m_subscribedTo.erase(it);
	}

	template <class ... TArgs>
	void Handler<TArgs...>::addEvent(EventT* e)
	{
		m_subscribedTo.push_back(e);
	}

	// Event implementation

	template <class ... TArgs>
	Event<TArgs...>::Event(Event<TArgs...>&& other)
	{
		swap(other);
	}

	template <class ... TArgs>
	Event<TArgs...>& Event<TArgs...>::operator=(Event<TArgs...>&& other)
	{
		swap(other);
		return *this;
	}

	template <class ... TArgs>
	Event<TArgs...>::~Event()
	{
		reset();
	}

	template <class ... TArgs>
	void Event<TArgs...>::invoke(TArgs... args) const
	{
		for (auto h : m_handler)
			h->invoke(args...);
	}

	template <class ... TArgs>
	void Event<TArgs...>::subscribe(HandlerT* handler)
	{
		subscribeConstHandler(handler);
		handler->addEvent(this);
	}

	template <class ... TArgs>
	void Event<TArgs...>::unsubscribe(HandlerT* handler)
	{
		auto it = std::find(m_handler.begin(), m_handler.end(), handler);
		if (it == m_handler.end()) return;
		(*it)->removeEvent(this);
		m_handler.erase(it);
	}

	template <class ... TArgs>
	void Event<TArgs...>::reset()
	{
		for (auto h : m_handler)
			h->removeEvent(this);
		m_handler.clear();
	}

	template <class ... TArgs>
	void Event<TArgs...>::swap(Event<TArgs...>& other)
	{
		// unsubscribe from old handlers
		for (auto h : m_handler)
			h->removeEvent(this);
		for (auto h : other.m_handler)
			h->removeEvent(&other);

		// swap data
		std::swap(m_handler, other.m_handler);

		// re-subscribe
		for (auto h : m_handler)
			h->addEvent(this);
		for (auto h : other.m_handler)
			h->addEvent(&other);
	}

	template <class ... TArgs>
	typename Event<TArgs...>::HandlerT Event<TArgs...>::subscribe(std::function<void(TArgs ...)> function)
	{
		HandlerT h(move(function));
		subscribe(&h);
		return h;
	}

	template <class ... TArgs>
	void Event<TArgs...>::subscribeConstHandler(HandlerT* handler)
	{
		m_handler.push_back(handler);
	}

	template <class ... TArgs>
	void Event<TArgs...>::unsubscribeConstHandler(const HandlerT* handler)
	{
		auto it = std::find(m_handler.begin(), m_handler.end(), handler);
		if (it == m_handler.end()) return;
		m_handler.erase(it);
	}
}


#pragma once
class LinkedBase
{
public:
    LinkedBase *linked_next = nullptr;
    LinkedBase *linked_previous = nullptr;
    virtual ~LinkedBase() = default; // Ensures proper destruction of derived classes
};

template <typename T>
class LinkedList
{
    // Handle both pointer and non-pointer types
    using element_type = std::remove_pointer_t<T>;
    static_assert(std::is_base_of_v<LinkedBase, element_type>, 
                 "T must inherit from LinkedBase or be pointer to type that inherits from LinkedBase");
    
    // Determine if T is a pointer type
    static constexpr bool is_pointer = std::is_pointer_v<T>;
    
    LinkedBase *head = nullptr;

    // Helper functions to convert between T and LinkedBase*
    LinkedBase* to_base(T node) {
        if constexpr (is_pointer) {
            return static_cast<LinkedBase*>(node);
        } else {
            return static_cast<LinkedBase*>(&node);
        }
    }
    
    T from_base(LinkedBase* base) {
        if constexpr (is_pointer) {
            return static_cast<T>(base);
        } else {
            return *static_cast<element_type*>(base);
        }
    }

public:
    void add(T node)
    {
        LinkedBase* base_node = to_base(node);
        
        if (head == nullptr)
        {
            head = base_node;
            base_node->linked_next = nullptr;
            base_node->linked_previous = nullptr;
        }
        else
        {
            base_node->linked_next = head;
            base_node->linked_previous = nullptr;
            head->linked_previous = base_node;
            head = base_node;
        }
    }

    void remove(T node)
    {
        LinkedBase* base_node = to_base(node);
        
        if (base_node == head)
        {
            head = base_node->linked_next;
            if (head != nullptr)
            {
                head->linked_previous = nullptr;
            }
        }
        else
        {
            if (base_node->linked_next != nullptr)
            {
                base_node->linked_next->linked_previous = base_node->linked_previous;
            }
            if (base_node->linked_previous != nullptr)
            {
                base_node->linked_previous->linked_next = base_node->linked_next;
            }
        }
        base_node->linked_next = nullptr;
        base_node->linked_previous = nullptr;
    }

    class Iterator
    {
        LinkedBase *current;
        bool is_iterator_pointer;

    public:
        Iterator(LinkedBase *current) : current(current) {}

        // Dereference returns T (either T& or T*)
        T operator*() const { 
            if constexpr (is_pointer) {
                return static_cast<T>(current);
            } else {
                return *static_cast<element_type*>(current);
            }
        }
        
        // Arrow operator returns T* (works for both pointer and non-pointer T)
        element_type* operator->() const { 
            return static_cast<element_type*>(current); 
        }

        // Increment
        Iterator &operator++()
        {
            current = current->linked_next;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator &other) const { return current == other.current; }
        bool operator!=(const Iterator &other) const { return current != other.current; }
    };

    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }
};
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
    static_assert(std::is_base_of_v<LinkedBase, T>, "T must inherit from LinkedBase");
    LinkedBase *head = nullptr;

public:
    void add(T *node)
    {
        if (head == nullptr)
        {
            head = node;
            node->linked_next = nullptr;
            node->linked_previous = nullptr;
        }
        else
        {
            node->linked_next = head;
            node->linked_previous = nullptr;
            head->linked_previous = node;
            head = node;
        }
    }

    void remove(T *node)
    {
        if (node == head)
        {
            head = node->linked_next;
            if (head != nullptr)
            {
                head->linked_previous = nullptr;
            }
        }
        else
        {
            if (node->linked_next != nullptr)
            {
                node->linked_next->linked_previous = node->linked_previous;
            }
            if (node->linked_previous != nullptr)
            {
                node->linked_previous->linked_next = node->linked_next;
            }
        }
        node->linked_next = nullptr;
        node->linked_previous = nullptr;
    }

    // Iterator that casts to T* automatically
    class Iterator
    {
        LinkedBase *current;

    public:
        Iterator(LinkedBase *current) : current(current) {}

        // Return T* instead of T&
        T *operator*() const { return static_cast<T *>(current); }
        T *operator->() const { return static_cast<T *>(current); }

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
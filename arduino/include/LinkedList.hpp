#pragma once
#include "Node.hpp"

template <class T>
class LinkedList
{
private:
  Node<T> *m_head;

public:
  LinkedList() : m_head(nullptr) {}

  T *getFront()
  {
    return m_head->getElement();
  }

  void addFront(T *element)
  {
    Node<T> *temp = new Node<T>(element, m_head);
    m_head = temp;
  }

  Node<T> *search(T *el)
  {
    Node<T> *temp = m_head;
    while (temp != nullptr)
    {
      if (*(temp->getElement()) == *el)
        break;

      temp = temp->getNext();
    }

    return temp;
  }

  Node<T> *searchNext(Node<T> *el)
  {
    Node<T> *temp = el;
    while (temp != nullptr)
    {
      if (*(temp->getElement()) == *el->getElement())
        break;

      temp = temp->getNext();
    }

    return temp;
  }
};

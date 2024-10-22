#pragma once

template <class T>
class Node
{
private:
  T *m_element;
  Node<T> *m_next;

public:
  Node() : m_element(nullptr), m_next(nullptr) {}

  explicit Node(T value) : m_element(value), m_next(nullptr) {}

  Node(T *value, Node<T> *next) : m_element(value), m_next(next) {}

  void setElement(T newE)
  {
    m_element = newE;
  }

  void setNext(Node<T> *newN)
  {
    m_next = newN;
  }

  T *getElement()
  {
    return m_element;
  }

  Node<T> *getNext()
  {
    return m_next;
  }
};
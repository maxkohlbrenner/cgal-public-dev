#ifndef SWEEP_LINE_SUBCURVE_NOTIFICATION
#define SWEEP_LINE_SUBCURVE_NOTIFICATION

#include <CGAL/Sweep_line_2/Sweep_line_event.h>
#include <CGAL/Sweep_line_2/Sweep_line_subcurve.h>

CGAL_BEGIN_NAMESPACE

template <class Traits,class OutputIerator>
class Sweep_line_subcurves_notification 
{
  typedef Sweep_line_subcurves_notification<Traits,OutputIerator>         Self;
  typedef Sweep_line_subcurve<Traits,Self>                                Subcurve;
  typedef Sweep_line_event<Traits, Subcurve, Self>            Event;
  typedef typename Traits::X_monotone_curve_2     X_monotone_curve_2;

  typedef Sweep_line_2_impl<Traits,
                            Event,
                            Subcurve,
                            Self,
                            CGAL_ALLOCATOR(int)> Sweep_line;



public:


  Sweep_line_subcurves_notification(OutputIerator out,
                                    bool overlapping): m_out(out),
                                                       m_overlapping(overlapping)
  {}

  void attach(Sweep_line *sl)
  {
    m_sweep_line = sl;
  }
       
  void before_handle_event(Event* event){}

  void after_handle_event(Event* event){}

  void add_subcurve(const X_monotone_curve_2& cv,Subcurve* sc)
  {
    if(!m_overlapping)
      *m_out++ = cv;  
    else
    {
      unsigned int overlap_depth = sc->overlap_depth();
      for(unsigned int i = 0 ; i < overlap_depth ; ++i)
        *m_out++ = cv;
    }
  }

  void init_subcurve(Subcurve* sc){}

  OutputIerator get_output_iterator()
  {
    return m_out;
  }
  


  protected:

    OutputIerator m_out;
    bool m_overlapping;
    Sweep_line *m_sweep_line;
  };

CGAL_END_NAMESPACE

#endif

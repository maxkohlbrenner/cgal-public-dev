// Copyright (c) 2009  GeometryFactory Sarl (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0+
//
//
// Author(s)     : Fernando Cacciola <Fernando.Cacciola@geometryfactory.com>
//                 Ronnie Gandhi <ronniegandhi19999@gmail.com>
//

#ifndef CGAL_QT_GRAPHICS_VIEW_BEZIER_POLYGON_INPUT_H
#define CGAL_QT_GRAPHICS_VIEW_BEZIER_POLYGON_INPUT_H

#include <CGAL/auto_link/Qt.h>
#include <CGAL/Qt/GraphicsViewInput.h>
#include <CGAL/Qt/Converter.h>
#include <CGAL/Arr_Bezier_curve_traits_2.h>

#include <QPolygonF>
#include <QPointF>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>


#include "QT5/BezierCurves.h"

namespace CGAL {
namespace Qt {

  template <class Traits_>
  class GraphicsViewBezierPolygonInput : public GraphicsViewInput
  {
  public:

    typedef Traits_ Traits ;
    typedef CGAL::Gps_traits_2<Traits>                    Bezier_gps_traits;
   // typedef CGAL::Gps_traits_2<Traits>                    Bezier_traits;
    typedef typename Traits::Curve_2                      Bezier_curve;
    typedef typename Traits::X_monotone_curve_2           Bezier_X_monotone_curve;
    typedef typename Bezier_gps_traits::General_polygon_2 Bezier_polygon;
   // typedef typename Bezier_traits::General_polygon_2     Bezier_polygon;
    typedef typename Traits::Rat_kernel::Vector_2         Vector ;
    typedef typename Traits::Rat_kernel::Point_2          Point ;

    typedef std::vector<Bezier_curve> Bezier_curve_vector ;

    typedef typename Bezier_curve_vector::const_iterator const_bezier_curve_iterator ;

    typedef Bezier_boundary_pieces_graphics_item<Bezier_curve_vector> GI ;

    GraphicsViewBezierPolygonInput(QObject* aParent, QGraphicsScene* aScene)
      :
        GraphicsViewInput( aParent         )
      , mScene           ( aScene          )
      , mState           ( Start           )
      , mBezierPolygonPen( QColor(0,255,0) )
      , mOngoingCurvePen ( QColor(255,0,0) )
      , mHandlePen       ( QColor(0,0,255) )
      , mBezierGI        ( 0               )
      , m_bound_rect     ( true            )
      , m_last_bezier    ( false           )
      , m_last           ( false           )
    {
      mOngoingPieceGI = new GI(&mOngoingPieceCtr) ;
      mHandle0GI      = new QGraphicsLineItem();
      mHandle1GI      = new QGraphicsLineItem();

      mOngoingPieceGI->setPen(mOngoingCurvePen);
      mHandle0GI     ->setPen(mHandlePen);
      mHandle1GI     ->setPen(mHandlePen);

      mHandle0GI->setLine(0,0,1,1);
      mHandle1GI->setLine(0,0,1,1);
      mHandle0GI->hide();
      mHandle1GI->hide();

      mBezierGI = new GI(&mBezierPolygonPieces) ;

      mBezierGI->setPen(mBezierPolygonPen);

      mScene->addItem(mOngoingPieceGI);
      mScene->addItem(mHandle0GI);
      mScene->addItem(mHandle1GI);
      mScene->addItem(mBezierGI);
    }

    ~GraphicsViewBezierPolygonInput(){}

    bool eventFilter(QObject *obj, QEvent *aEvent)
    {
      bool rHandled = false ;

      if (aEvent->type() == QEvent::GraphicsSceneMousePress)
      {
        rHandled = mousePressEvent( static_cast<QGraphicsSceneMouseEvent *>(aEvent) ) ;
      }
      else if (aEvent->type() == QEvent::GraphicsSceneMouseRelease)
      {
        rHandled = mouseReleaseEvent( static_cast<QGraphicsSceneMouseEvent *>(aEvent) ) ;
      }
      else if (aEvent->type() == QEvent::GraphicsSceneMouseMove)
      {
        rHandled = mouseMoveEvent( static_cast<QGraphicsSceneMouseEvent *>(aEvent) ) ;
      }
      else if (aEvent->type() == QEvent::KeyPress)
      {
        rHandled = keyPressEvent( static_cast<QKeyEvent *>(aEvent) ) ;
      }

      if ( !rHandled )
        rHandled = QObject::eventFilter(obj, aEvent);

      return rHandled ;
    }

  public:

    enum State { Start, PieceOrFirstHandleStarted, PieceOngoing, FirstHandleOngoing, HandleOngoing, PieceEnded, CurveEnded } ;

    Point cvt ( QPointF const& aP ) const { return Point(aP.x(),aP.y()) ; }

    bool mousePressEvent(QGraphicsSceneMouseEvent *aEvent)
    {

      bool rHandled = false;
      m_bound_rect = false;

      Point lP = cvt(aEvent->scenePos());

      if ( aEvent->button() == ::Qt::LeftButton )
      {
        switch (mState)
        {
          case Start:
            mP0      = lP;
            mState   = PieceOrFirstHandleStarted;
            rHandled = true;
            break;

          case PieceOngoing:
            mP1      = lP;
            mState   = HandleOngoing;
            rHandled = true;
            break;

            default: break; //!todo handle default case
        }
      }

      else  if (aEvent->button() == ::Qt::RightButton) {
        switch (mState) {
          case PieceOngoing:
            // allowing user to curve last piece as well
            m_last = true;
            mState = HandleOngoing;
            rHandled = true;
            break;

           default: break; //! \todo handle default case
         }
    }

      return rHandled ;
    }

    bool mouseMoveEvent(QGraphicsSceneMouseEvent *aEvent)
    {
      bool rHandled = false ;
      Point lP = cvt(aEvent->scenePos());

      switch (mState)
      {
        case PieceOrFirstHandleStarted:
          mState   = FirstHandleOngoing;
          rHandled = true;
          break;

        case PieceOngoing:
          mP1 = lP;
          UpdateOngoingPiece();
          rHandled = true ;
          break;

        case FirstHandleOngoing:
          UpdateVeryFirstHandle(lP);
          rHandled = true ;
          break;

        case HandleOngoing:
          if(m_last)
          {
            mP1 = mBezierPolygonPieces.front().control_point(0);
            m_last_bezier = true;
          }
          UpdateHandles(lP);
          UpdateOngoingPiece();
          rHandled = true ;
          break;

        case PieceEnded:
          mState   = PieceOngoing;
          rHandled = true;
          break;
      }

      return rHandled ;
    }

    bool mouseReleaseEvent(QGraphicsSceneMouseEvent *aEvent)
    {
      bool rHandled = false ;

      Point lP = cvt(aEvent->scenePos());

      if ( aEvent->button() == ::Qt::LeftButton )
      {
        switch (mState)
        {
          case PieceOrFirstHandleStarted:
            mState   = PieceOngoing;
            rHandled = true;
            break;

          case FirstHandleOngoing:
            UpdateVeryFirstHandle(lP);
            mPrevH0  = mH1 ;
            mH1      = std::optional<Point>();
            mState   = PieceOngoing;
            rHandled = true;
            break;

          case HandleOngoing:
            UpdateHandles(lP);
            CommitOngoingPiece(lP);
            mState   = PieceEnded;
            rHandled = true;
            break;

            default: break; //!todo add default case handling
        }
      }
      else if ( aEvent->button() == ::Qt::RightButton )
      {
        switch (mState)
        {
          case HandleOngoing:
            m_bound_rect = false;
            if(m_last_bezier)
            {
              HideHandles();
              CommitOngoingPiece(lP);
            }
            CloseCurrBundary();
            CommitCurrBezierPolygon();
            ReStart();
            rHandled = true;
            break;

            default: break; //!todo handle default case
        }
      }
      return rHandled ;
    }

    bool keyPressEvent(QKeyEvent *aEvent)
    {
      bool rHandled = false ;

      if( aEvent->key() == ::Qt::Key_Delete || aEvent->key() == ::Qt::Key_Backspace )
      {
        RemoveLastPiece();
        mState   = mBezierPolygonPieces.size() > 0 ? PieceEnded : Start ;
        rHandled = true;
      }
      else if( aEvent->key() == ::Qt::Key_Escape)
      {
        Reset();
        mState   = Start;
        rHandled = true;
      }
      return rHandled ;
    }


  public:

    Bezier_curve const* ongoing_piece() const { return mOngoingPieceCtr.size() == 1 ? &mOngoingPieceCtr[0] : NULL ; }

    void ReStart()
    {
      mPrevH0 = mH0 = mH1 = std::optional<Point>();
      mState = Start ;
    }

    void Reset()
    {
      mBezierPolygonPieces.clear();
      mOngoingPieceCtr    .clear();
      mBezierGI      ->modelChanged();
      mOngoingPieceGI->modelChanged();
      ReStart();
    }

    void RemoveLastPiece()
    {
      mBezierPolygonPieces.pop_back();
      mOngoingPieceCtr      .clear();
      mBezierGI      ->modelChanged();
      mOngoingPieceGI->modelChanged();
      if ( mBezierPolygonPieces.size() > 0 )
      {
        mP0 = mBezierPolygonPieces.back().control_point(mBezierPolygonPieces.back().number_of_control_points()-1);
        UpdateOngoingPiece();
      }
      mPrevH0 = mH0 = mH1 = std::optional<Point>();
    }

    void HideHandles()
    {
      mHandle0GI->hide();
      mHandle1GI->hide();
    }

    Bezier_curve CreatePiece()
    {
      if ( mPrevH0 && mH1 && *mPrevH0 != *mH1 && *mPrevH0 != mP0 && *mH1 != mP1 )
      {
        Point lControlPoints[4] = { mP0
                                  , *mPrevH0
                                  , *mH1
                                  , mP1
                                  } ;
        return Bezier_curve( lControlPoints, lControlPoints + 4 ) ;
      }
      else if ( mPrevH0 && !mH1 && *mPrevH0 != mP0 && *mPrevH0 != mP1 )
      {
        Point lControlPoints[3] = { mP0
                                  , *mPrevH0
                                  , mP1
                                  } ;
        return Bezier_curve( lControlPoints, lControlPoints + 3 ) ;
      }
      else if ( !mPrevH0 && mH1 && *mH1 != mP0 && *mH1 != mP1 )
      {
        Point lControlPoints[3] = { mP0
                                  , *mH1
                                  , mP1
                                  } ;
        return Bezier_curve( lControlPoints, lControlPoints + 3 ) ;
      }
      else
      {
        Point lControlPoints[2] = { mP0
                                  , mP1
                                  } ;
        return Bezier_curve( lControlPoints, lControlPoints + 2 ) ;
      }
    }

    void UpdateOngoingPiece()
    {
      if ( mOngoingPieceCtr.size() > 0 )
           mOngoingPieceCtr.clear();
           mOngoingPieceCtr.push_back(CreatePiece());
           mOngoingPieceGI->modelChanged();
    }

    void CommitOngoingPiece( Point const& aP )
    {
      if ( ongoing_piece() )
      {
        mBezierPolygonPieces.push_back( *ongoing_piece() ) ;
        mBezierGI->modelChanged();
        mOngoingPieceCtr.clear();
        mOngoingPieceGI->modelChanged();
        mP0 = mP1 ;
        mP1 = aP ;
        mPrevH0 = mH0 ;
        mH0 = mH1 = std::optional<Point>();
      }
    }

    void UpdateVeryFirstHandle(Point const& aP)
    {
      if ( squared_distance(mP0,aP) >= 9 )
      {
        mH1 = aP ;
        mHandle1GI->setLine( to_double(mP0.x()), to_double(mP0.y()),
                             to_double(mH1->x()), to_double(mH1->y()));
        mHandle1GI->show();

        mH0 = std::optional<Point>();
        mHandle0GI->hide();
      }
      else
      {
        HideHandles();
        mH0 = mH1 = std::optional<Point>();
      }
    }

    void UpdateHandles(Point const& aP)
    {
      if ( squared_distance(mP1,aP) >= 9 )
      {
        mH0 = aP ;
        mH1 = mP1 - (aP - mP1);

        mHandle0GI->setLine( to_double(mP1.x()), to_double(mP1.y()), to_double(mH0->x()), to_double(mH0->y()));
        mHandle1GI->setLine( to_double(mP1.x()), to_double(mP1.y()), to_double(mH1->x()), to_double(mH1->y()));
        mHandle0GI->show();
        mHandle1GI->show();
      }
      else
      {
        HideHandles();
        mH0 = mH1 = std::optional<Point>();
      }
    }

    void CloseCurrBundary()
    {
      if ( mBezierPolygonPieces.size() > 0 && ongoing_piece()!= NULL  && !m_last_bezier)
      {
        std::vector<Point> lControlPoints(ongoing_piece()->control_points_begin(),ongoing_piece()->control_points_end());

        lControlPoints.back() = mBezierPolygonPieces.front().control_point(0);

        mBezierPolygonPieces.push_back( Bezier_curve( lControlPoints.begin(), lControlPoints.end() ) ) ;

        mBezierGI->modelChanged() ;
      }
    }

    void CommitCurrBezierPolygon()
    {
      GenerateBezierPolygon();

      mOngoingPieceCtr.clear();
      mOngoingPieceGI->modelChanged();

      mBezierPolygonPieces.clear();
      mBezierGI->modelChanged() ;

      mPrevH0 = mH0 = mH1 = std::optional<Point>();

      HideHandles();
    }

    void GenerateBezierPolygon() {
      Traits traits;
      auto make_x_monotone = traits.make_x_monotone_2_object();
      using Make_x_monotone_result =
        std::variant<typename Traits::Point_2, Bezier_X_monotone_curve>;

      std::vector<Bezier_X_monotone_curve> xcvs;

      for (auto it = mBezierPolygonPieces.begin();
           it != mBezierPolygonPieces.end(); ++ it) {
        std::vector<Make_x_monotone_result> x_objs;
        make_x_monotone(*it, std::back_inserter(x_objs));
        for (auto i = 0; i < x_objs.size(); ++i) {
            auto* xcv = std::get_if<Bezier_X_monotone_curve>(&x_objs[i]);
            CGAL_assertion(xcv != nullptr);
            xcvs.push_back(*xcv);
        }
      }

      if ( xcvs.size() > 0 )
      {
        m_last = false;
        m_last_bezier = false;
        Bezier_polygon bp(xcvs.begin(), xcvs.end());
        emit(generate(std::variant<std::pair<Bezier_polygon, Bezier_curve_vector> >(std::make_pair(bp,mBezierPolygonPieces))));
      }
    }

    void get_BoundingRect()
    {

      m_bound_rect = true;

      mP0 = Point(-15500000,-10000000);
      mState  = PieceOrFirstHandleStarted;

      mState   = PieceOngoing;
      mP1 = Point(-15500000,10000000);
      UpdateOngoingPiece();

      mP1 = Point(-15500000,10000000);
      mState = HandleOngoing;
      UpdateHandles(Point(-15500000,10000000));
      CommitOngoingPiece(Point(-15500000,10000000));
      mState   = PieceEnded;

      mState   = PieceOngoing;
      mP1 = Point(15500000,10000000);
      UpdateOngoingPiece();

      mP1 = Point(15500000,10000000);
      mState = HandleOngoing;
      UpdateHandles(Point(15500000,10000000));
      CommitOngoingPiece(Point(15500000,10000000));
      mState   = PieceEnded;

      mState   = PieceOngoing;
      mP1 = Point(15500000,-10000000);
      UpdateOngoingPiece();

      mP1 = Point(15500000,-10000000);
      mState = HandleOngoing;
      UpdateHandles(Point(15500000,-10000000));
      CommitOngoingPiece(Point(15500000,-10000000));
      mState   = PieceEnded;

      mState   = PieceOngoing;
      mP1 = Point(-9000000,-9000000);
      UpdateOngoingPiece();

      CloseCurrBundary();
      CommitCurrBezierPolygon();
      ReStart();
    }

    bool isboundingRect()
    {
      return m_bound_rect;
    }

  public:
    QGraphicsScene*    mScene ;
    GI*                mBezierGI ;
    GI*                mOngoingPieceGI ;
    QGraphicsLineItem* mHandle0GI ;
    QGraphicsLineItem* mHandle1GI ;

    QPen mBezierPolygonPen ;
    QPen mOngoingCurvePen ;
    QPen mHandlePen ;

    bool m_bound_rect;
    bool m_last_bezier;
    bool m_last;

    Bezier_curve_vector mBezierPolygonPieces ;
    Bezier_curve_vector mOngoingPieceCtr ;

    int mState;

    Point mP0;
    Point mP1;

    std::optional<Point> mPrevH0;
    std::optional<Point> mH0;
    std::optional<Point> mH1;

  }; // end class GraphicsViewBezierPolygonInput

} // namespace Qt
} // namespace CGAL

#endif // CGAL_QT_GRAPHICS_VIEW_BEZIER_REGION_INPUT_H

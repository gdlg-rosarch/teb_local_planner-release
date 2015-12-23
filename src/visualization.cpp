/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2015,
 *  TU Dortmund - Institute of Control Theory and Systems Engineering.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the institute nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Christoph Rösmann
 *********************************************************************/

#include <teb_local_planner/visualization.h>
#include <teb_local_planner/optimal_planner.h>


namespace teb_local_planner
{

TebVisualization::TebVisualization() : initialized_(false)
{
}

TebVisualization::TebVisualization(ros::NodeHandle& nh, const TebConfig& cfg) : initialized_(false)
{
  initialize(nh, cfg);
}

void TebVisualization::initialize(ros::NodeHandle& nh, const TebConfig& cfg)
{
  if (initialized_)
    ROS_WARN("TebVisualization already initialized. Reinitalizing...");
  
  // set config
  cfg_ = &cfg;
  
  // register topics
  global_plan_pub_ = nh.advertise<nav_msgs::Path>("global_plan", 1);
  local_plan_pub_ = nh.advertise<nav_msgs::Path>("local_plan",1);
  teb_poses_pub_ = nh.advertise<geometry_msgs::PoseArray>("teb_poses", 100);
  teb_marker_pub_ = nh.advertise<visualization_msgs::Marker>("teb_markers", 1000);
    
  initialized_ = true; 
}



void TebVisualization::publishGlobalPlan(const std::vector<geometry_msgs::PoseStamped>& global_plan) const
{
  if ( printErrorWhenNotInitialized() ) return;
  base_local_planner::publishPlan(global_plan, global_plan_pub_); 
}

void TebVisualization::publishLocalPlan(const std::vector<geometry_msgs::PoseStamped>& local_plan) const
{
  if ( printErrorWhenNotInitialized() )
    return;
  base_local_planner::publishPlan(local_plan, local_plan_pub_); 
}

void TebVisualization::publishLocalPlanAndPoses(const TimedElasticBand& teb) const
{
  if ( printErrorWhenNotInitialized() )
    return;
  
    // create path msg
    nav_msgs::Path teb_path;
    teb_path.header.frame_id = cfg_->map_frame;
    teb_path.header.stamp = ros::Time::now();
    
    // create pose_array (along trajectory)
    geometry_msgs::PoseArray teb_poses;
    teb_poses.header.frame_id = teb_path.header.frame_id;
    teb_poses.header.stamp = teb_path.header.stamp;
    
    // fill path msgs with teb configurations
    for (unsigned int i=0; i < teb.sizePoses(); i++)
    {
      geometry_msgs::PoseStamped pose;
      pose.header.frame_id = teb_path.header.frame_id;
      pose.header.stamp = teb_path.header.stamp;
      pose.pose.position.x = teb.Pose(i).x();
      pose.pose.position.y = teb.Pose(i).y();
      pose.pose.position.z = 0;
      pose.pose.orientation = tf::createQuaternionMsgFromYaw(teb.Pose(i).theta());
      teb_path.poses.push_back(pose);
      teb_poses.poses.push_back(pose.pose);
    }
    local_plan_pub_.publish(teb_path);
    teb_poses_pub_.publish(teb_poses);
}

void TebVisualization::publishObstacles(const ObstContainer& obstacles) const
{
  if ( printErrorWhenNotInitialized() )
    return;
  
  // Visualize point obstacles
  {
    visualization_msgs::Marker marker;
    marker.header.frame_id = cfg_->map_frame;
    marker.header.stamp = ros::Time::now();
    marker.ns = "PointObstacles";
    marker.id = 0;
    marker.type = visualization_msgs::Marker::POINTS;
    marker.action = visualization_msgs::Marker::ADD;
    
    for (ObstContainer::const_iterator obst = obstacles.begin(); obst != obstacles.end(); ++obst)
    {
      boost::shared_ptr<PointObstacle> pobst = boost::dynamic_pointer_cast<PointObstacle>(*obst);      
      if (!pobst)
        continue;
      geometry_msgs::Point point;
      point.x = pobst->x();
      point.y = pobst->y();
      point.z = 0;
      marker.points.push_back(point);
    }
    
    marker.scale.x = 0.1;
    marker.scale.y = 0.1;
    marker.color.a = 1.0;
    marker.color.r = 1.0;
    marker.color.g = 0.0;
    marker.color.b = 0.0;

    teb_marker_pub_.publish( marker );
  }
  
  // Visualize line obstacles
  {
    unsigned int idx = 0;
    for (ObstContainer::const_iterator obst = obstacles.begin(); obst != obstacles.end(); ++obst)
    {	
      boost::shared_ptr<LineObstacle> pobst = boost::dynamic_pointer_cast<LineObstacle>(*obst);   
      if (!pobst)
        continue;
      
      visualization_msgs::Marker marker;
      marker.header.frame_id = cfg_->map_frame;
      marker.header.stamp = ros::Time::now();
      marker.ns = "LineObstacles";
      marker.id = idx++;
      marker.type = visualization_msgs::Marker::LINE_STRIP;
      marker.action = visualization_msgs::Marker::ADD;
      marker.lifetime = ros::Duration(5.0);
      geometry_msgs::Point start;
      start.x = pobst->start().x();
      start.y = pobst->start().y();
      start.z = 0;
      marker.points.push_back(start);
      geometry_msgs::Point end;
      end.x = pobst->end().x();
      end.y = pobst->end().y();
      end.z = 0;
      marker.points.push_back(end);
  
      marker.scale.x = 0.1;
      marker.scale.y = 0.1;
      marker.color.a = 1.0;
      marker.color.r = 0.0;
      marker.color.g = 1.0;
      marker.color.b = 0.0;
      
      teb_marker_pub_.publish( marker );     
    }
  }
  

  // Visualize polygon obstacles
  {
    unsigned int idx = 0;
    for (ObstContainer::const_iterator obst = obstacles.begin(); obst != obstacles.end(); ++obst)
    {	
      boost::shared_ptr<PolygonObstacle> pobst = boost::dynamic_pointer_cast<PolygonObstacle>(*obst);   
      if (!pobst)
				continue;
      
      visualization_msgs::Marker marker;
      marker.header.frame_id = cfg_->map_frame;
      marker.header.stamp = ros::Time::now();
      marker.ns = "PolyObstacles";
      marker.id = idx++;
      marker.type = visualization_msgs::Marker::LINE_STRIP;
      marker.action = visualization_msgs::Marker::ADD;
      
      for (PolygonObstacle::VertexContainer::const_iterator vertex = pobst->vertices().begin(); vertex != pobst->vertices().end(); ++vertex)
      {
        geometry_msgs::Point point;
        point.x = vertex->x();
        point.y = vertex->y();
        point.z = 0;
        marker.points.push_back(point);
      }
      
      // Also add last point to close the polygon
      // but only if polygon has more than 2 points (it is not a line)
      if (pobst->vertices().size() > 2)
      {
        geometry_msgs::Point point;
        point.x = pobst->vertices().front().x();
        point.y = pobst->vertices().front().y();
        point.z = 0;
        marker.points.push_back(point);
      }
      marker.scale.x = 0.1;
      marker.scale.y = 0.1;
      marker.color.a = 1.0;
      marker.color.r = 1.0;
      marker.color.g = 0.0;
      marker.color.b = 0.0;
      
      teb_marker_pub_.publish( marker );     
    }
  }
}


void TebVisualization::publishTebContainer(const TebOptPlannerContainer& teb_planner, const std::string& ns)
{
if ( printErrorWhenNotInitialized() )
    return;
  
  visualization_msgs::Marker marker;
  marker.header.frame_id = cfg_->map_frame;
  marker.header.stamp = ros::Time::now();
  marker.ns = ns;
  marker.id = 0;
  marker.type = visualization_msgs::Marker::LINE_LIST;
  marker.action = visualization_msgs::Marker::ADD;
  
  // Iterate through teb pose sequence
  for( TebOptPlannerContainer::const_iterator it_teb = teb_planner.begin(); it_teb != teb_planner.end(); ++it_teb )
  {	  
    // iterate single poses
    PoseSequence::const_iterator it_pose = it_teb->get()->teb().poses().begin();
    PoseSequence::const_iterator it_pose_end = it_teb->get()->teb().poses().end();
    std::advance(it_pose_end, -1); // since we are interested in line segments, reduce end iterator by one.
    while (it_pose != it_pose_end) 
    {
      geometry_msgs::Point point_start;
      point_start.x = (*it_pose)->x();
      point_start.y = (*it_pose)->y();
      point_start.z = 0;
      marker.points.push_back(point_start);

      geometry_msgs::Point point_end;
      point_end.x = (*boost::next(it_pose))->x();
      point_end.y = (*boost::next(it_pose))->y();
      point_end.z = 0;
      marker.points.push_back(point_end);
      ++it_pose;
    }
  }
  marker.scale.x = 0.01;
  marker.color.a = 1.0;
  marker.color.r = 0.5;
  marker.color.g = 1.0;
  marker.color.b = 0.0;

  teb_marker_pub_.publish( marker );
}


inline bool TebVisualization::printErrorWhenNotInitialized() const
{
  if (!initialized_)
  {
    ROS_ERROR("TebVisualization class not initialized. You must call initialize or an appropriate constructor");
    return true;
  }
  return false;
}

} // namespace teb_local_planner

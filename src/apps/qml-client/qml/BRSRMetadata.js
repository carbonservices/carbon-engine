.pragma library

// Comprehensive BRSR (Business Responsibility & Sustainability Reporting) metadata dictionary
// Maps technical snake_case schema keys to authoritative, non-technical SEBI ESG consultant labels,
// and provides column definitions for all 45+ loop/tabular disclosure tables.

var fieldLabels = {
    // Section A: Corporate Identity & Capital
    "entity_cin": "Corporate Identification Number (CIN)",
    "entity_name": "Name of the Company / Entity",
    "year_of_incorporation": "Year of Incorporation",
    "registered_address": "Registered Office Address",
    "corporate_address": "Corporate Office Address",
    "contact_email": "E-mail Address",
    "contact_telephone": "Telephone / Contact Number",
    "website_url": "Website URL",
    "reporting_financial_year": "Reporting Financial Year",
    "stock_exchanges": "Stock Exchanges Where Listed",
    "paid_up_capital": "Paid-up Capital (₹)",
    "contact_person_details": "Name & Contact of Reporting Officer",
    "reporting_boundary": "Reporting Boundary (Standalone / Consolidated)",
    "assurance_provider_name": "Name of Assurance Provider",
    "assurance_type": "Type of Assurance (Limited / Reasonable)",
    "current_fy": "Current Financial Year",
    "previous_fy": "Previous Financial Year",
    "prior_to_previous_fy": "Prior to Previous Financial Year",
    "customer_types": "Customer Categories / Segments",

    // Section A: Operations, Markets & CSR
    "national_plants": "Plants / Manufacturing Units (National)",
    "national_offices": "Offices (National)",
    "national_total": "Total National Operational Locations",
    "international_plants": "Plants / Manufacturing Units (International)",
    "international_offices": "Offices (International)",
    "international_total": "Total International Operational Locations",
    "markets_national_states": "States Served in India",
    "markets_international_countries": "Countries Served Globally",
    "export_contribution_pct": "Contribution of Exports to Turnover (%)",
    "csr_applicable": "CSR Applicability (Section 135)",
    "csr_turnover_amount": "Turnover for CSR Calculation (₹)",
    "csr_net_worth": "Net Worth for CSR Calculation (₹)",
    "csr_obligation_amount": "CSR Obligation Amount (₹)",
    "csr_spent_amount": "Total CSR Amount Spent (₹)",
    "csr_unspent_amount": "CSR Unspent Amount (₹)",
    "esg_assessment_done": "External ESG Assessment Conducted?",
    "esg_assessment_agency": "ESG Assessment Agency / Framework",
    "esg_assessment_scope": "Scope of External ESG Assessment",

    // Section A: Employees & Governance Diversity
    "emp_perm_total": "Permanent Employees - Total",
    "emp_perm_male_no": "Permanent Employees - Male (No.)",
    "emp_perm_male_pct": "Permanent Employees - Male (%)",
    "emp_perm_female_no": "Permanent Employees - Female (No.)",
    "emp_perm_female_pct": "Permanent Employees - Female (%)",
    "emp_other_total": "Other than Permanent Employees - Total",
    "emp_other_male_no": "Other than Permanent - Male (No.)",
    "emp_other_male_pct": "Other than Permanent - Male (%)",
    "emp_other_female_no": "Other than Permanent - Female (No.)",
    "emp_other_female_pct": "Other than Permanent - Female (%)",
    "emp_total_total": "Total Employees (No.)",
    "emp_total_male_no": "Total Employees - Male (No.)",
    "emp_total_male_pct": "Total Employees - Male (%)",
    "emp_total_female_no": "Total Employees - Female (No.)",
    "emp_total_female_pct": "Total Employees - Female (%)",
    "board_total": "Board of Directors - Total",
    "board_female_no": "Board of Directors - Female (No.)",
    "board_female_pct": "Board of Directors - Female (%)",
    "kmp_total": "Key Managerial Personnel (KMP) - Total",
    "kmp_female_no": "Key Managerial Personnel - Female (No.)",
    "kmp_female_pct": "Key Managerial Personnel - Female (%)",

    // Section A: Workers & Differently-Abled
    "wrk_perm_total": "Permanent Workers - Total",
    "wrk_perm_male_no": "Permanent Workers - Male (No.)",
    "wrk_perm_male_pct": "Permanent Workers - Male (%)",
    "wrk_perm_female_no": "Permanent Workers - Female (No.)",
    "wrk_perm_female_pct": "Permanent Workers - Female (%)",
    "wrk_other_total": "Other than Permanent Workers - Total",
    "wrk_other_male_no": "Other Workers - Male (No.)",
    "wrk_other_male_pct": "Other Workers - Male (%)",
    "wrk_other_female_no": "Other Workers - Female (No.)",
    "wrk_other_female_pct": "Other Workers - Female (%)",
    "wrk_total_total": "Total Workers (No.)",
    "wrk_total_male_no": "Total Workers - Male (No.)",
    "wrk_total_male_pct": "Total Workers - Male (%)",
    "wrk_total_female_no": "Total Workers - Female (No.)",
    "wrk_total_female_pct": "Total Workers - Female (%)",
    "da_emp_perm_total": "Differently Abled Permanent Employees - Total",
    "da_emp_perm_male_no": "Differently Abled Perm Employees - Male (No.)",
    "da_emp_perm_male_pct": "Differently Abled Perm Employees - Male (%)",
    "da_emp_perm_female_no": "Differently Abled Perm Employees - Female (No.)",
    "da_emp_perm_female_pct": "Differently Abled Perm Employees - Female (%)",
    "da_emp_other_total": "Differently Abled Other Employees - Total",
    "da_emp_other_male_no": "Differently Abled Other Employees - Male (No.)",
    "da_emp_other_male_pct": "Differently Abled Other Employees - Male (%)",
    "da_emp_other_female_no": "Differently Abled Other Employees - Female (No.)",
    "da_emp_other_female_pct": "Differently Abled Other Employees - Female (%)",
    "da_emp_total": "Total Differently Abled Employees",
    "da_emp_male_no": "Total Differently Abled Employees - Male (No.)",
    "da_emp_male_pct": "Total Differently Abled Employees - Male (%)",
    "da_emp_female_no": "Total Differently Abled Employees - Female (No.)",
    "da_emp_female_pct": "Total Differently Abled Employees - Female (%)",
    "da_wrk_perm_total": "Differently Abled Permanent Workers - Total",
    "da_wrk_perm_male_no": "Differently Abled Perm Workers - Male (No.)",
    "da_wrk_perm_male_pct": "Differently Abled Perm Workers - Male (%)",
    "da_wrk_perm_female_no": "Differently Abled Perm Workers - Female (No.)",
    "da_wrk_perm_female_pct": "Differently Abled Perm Workers - Female (%)",
    "da_wrk_other_total": "Differently Abled Other Workers - Total",
    "da_wrk_other_male_no": "Differently Abled Other Workers - Male (No.)",
    "da_wrk_other_male_pct": "Differently Abled Other Workers - Male (%)",
    "da_wrk_other_female_no": "Differently Abled Other Workers - Female (No.)",
    "da_wrk_other_female_pct": "Differently Abled Other Workers - Female (%)",
    "da_wrk_total": "Total Differently Abled Workers",
    "da_wrk_male_no": "Total Differently Abled Workers - Male (No.)",
    "da_wrk_male_pct": "Total Differently Abled Workers - Male (%)",
    "da_wrk_female_no": "Total Differently Abled Workers - Female (No.)",
    "da_wrk_female_pct": "Total Differently Abled Workers - Female (%)",

    // Section B: Governance & Oversight
    "director_esg_statement": "Director / Executive Statement on ESG Strategy",
    "highest_authority_details": "Highest Authority Responsible for BRSR Implementation",
    "sustainability_committee_details": "Board Sustainability Committee Mandate & Composition",
    "governance_oversight_statement": "Governance Oversight & Review Process",
    "sustainability_decision_committee_details": "Decision Making Committee Details",

    // Principle 1: Ethics, Transparency & Accountability
    "p1_accounts_payable_days": "Average Accounts Payable Days",
    "p1_value_chain_awareness": "Value Chain Programs on Ethics & Anti-Corruption",
    "p1_conflict_of_interest_process": "Conflict of Interest Mechanism for Directors & KMP",

    // Principle 2: Sustainable Goods & Services
    "p2_rd_pct_current": "R&D Expenditure in Current FY (%)",
    "p2_rd_pct_prev": "R&D Expenditure in Previous FY (%)",
    "p2_capex_pct_current": "Capex for Environmental & Social Upgrades - Current FY (%)",
    "p2_capex_pct_prev": "Capex for Environmental & Social Upgrades - Previous FY (%)",
    "p2_sustainable_sourcing_process": "Procedures for Sustainable Sourcing",
    "p2_sustainable_sourcing_pct": "Total Inputs Sourced Sustainably (%)",
    "p2_rd_improvements": "R&D Initiatives for Resource Efficiency",
    "p2_capex_improvements": "Capex Investments in Clean Technologies",
    "p2_epr_status": "Extended Producer Responsibility (EPR) Plan & Status",
    "p2_adverse_impact_mitigation": "Mitigation of Adverse Environmental & Social Impacts",

    // Principle 3: Employee Wellbeing & Safety
    "p3_ohs_system_implemented": "Occupational Health & Safety (OHS) System Coverage",
    "p3_wc_filed_current": "Working Conditions Complaints Filed (Current FY)",
    "p3_wc_pending_current": "Working Conditions Complaints Pending (Current FY)",
    "p3_wc_filed_prev": "Working Conditions Complaints Filed (Previous FY)",
    "p3_wc_pending_prev": "Working Conditions Complaints Pending (Previous FY)",
    "p3_hs_filed_current": "Health & Safety Complaints Filed (Current FY)",
    "p3_hs_pending_current": "Health & Safety Complaints Pending (Current FY)",
    "p3_hs_filed_prev": "Health & Safety Complaints Filed (Previous FY)",
    "p3_hs_pending_prev": "Health & Safety Complaints Pending (Previous FY)",
    "p3_accessibility_details": "Workplace Accessibility for Differently Abled Persons",
    "p3_equal_opportunity_policy": "Equal Opportunity Policy & Inclusivity Framework",
    "p3_hazard_identification_process": "Work-Related Hazards Identification & Risk Assessment",
    "p3_hazard_reporting_process": "Worker Hazard Reporting & Protection from Retaliation",
    "p3_healthcare_access": "Non-Occupational Medical & Healthcare Services Access",
    "p3_safe_workplace_measures": "Measures for Safe & Healthy Workplace",
    "p3_value_chain_assessment": "Value Chain Assessment on Health & Safety",
    "p3_corrective_actions": "Corrective Actions on Health & Safety Audits",
    "p3_death_compensation_details": "Statutory Rehabilitation & Compensation for Work Deaths",
    "p3_disability_inclusion_measures": "Disability Inclusion & Rehabilitation Programs",
    "p3_value_chain_parental_leave": "Parental Leave Support across Value Chain",
    "p3_value_chain_grievance_mechanism": "Value Chain Worker Grievance Mechanism",
    "p3_transition_assistance_programs": "Transition & Reskilling Programs for Employees",

    // Principle 4: Stakeholder Engagement
    "p4_stakeholder_identification_process": "Process for Identifying Key Stakeholder Groups",
    "p4_consultation_process": "Stakeholder Consultation & Engagement Channels",
    "p4_vulnerable_groups_consultation": "Engagement with Vulnerable and Marginalized Stakeholders",
    "p4_consultation_outcomes": "Key Issues Raised & Management Action Taken",

    // Principle 5: Human Rights
    "p5_focal_point_details": "Human Rights Focal Point / Committee",
    "p5_grievance_mechanism": "Human Rights Grievance Redressal Mechanism",
    "p5_anti_retaliation_mechanism": "Whistleblower & Anti-Retaliation Protection",
    "p5_hr_in_contracts": "Human Rights Clauses in Business Agreements",
    "p5_corrective_actions": "Corrective Actions for Human Rights Grievances",
    "p5_process_modification": "Business Process Modifications to Prevent Violations",
    "p5_value_chain_hr_assessment": "Human Rights Due Diligence Across Value Chain",

    // Principle 6: Environmental Performance
    "current_renewable_energy_pct": "Renewable Energy Share - Current FY (%)",
    "previous_renewable_energy_pct": "Renewable Energy Share - Previous FY (%)",
    "scope_1_2_intensity_current": "Scope 1 & 2 GHG Intensity per Revenue (Current FY)",
    "scope_1_2_intensity_prev": "Scope 1 & 2 GHG Intensity per Revenue (Previous FY)",
    "p6_ghg_reduction_projects": "GHG Reduction Initiatives & Clean Energy Projects",
    "p6_low_carbon_transition_plan": "Low Carbon Transition Plan & Net-Zero Target",
    "p6_water_intensity_turnover_current": "Water Intensity per Turnover (Current FY)",
    "p6_water_intensity_turnover_prev": "Water Intensity per Turnover (Previous FY)",
    "p6_value_chain_env_assessment_pct": "Value Chain Partners Evaluated for Environment (%)",
    "p6_pat_scheme_details": "Perform Achieve and Trade (PAT) Energy Saving Target",
    "p6_zld_status": "Zero Liquid Discharge (ZLD) Implementation Status",
    "p6_water_stress_area_discharge": "Effluent Discharge in Water-Stressed Areas",
    "p6_environmental_compliance": "Compliance with Applicable Environmental Laws",
    "p6_circular_economy_initiatives": "Circular Economy & Waste Minimization Programs",
    "p6_sustainability_projects": "Significant Environmental Capital Projects",
    "p6_business_continuity_plan": "Environmental Emergency Preparedness & BCP",
    "p6_value_chain_adverse_impact": "Adverse Environmental Impacts Identified in Value Chain",
    "p6_value_chain_corrective_actions": "Corrective Action Plan for Value Chain Impacts",
    "p6_certifications_audits": "Environmental Management Certifications (ISO 14001, etc.)",
    "p6_climate_bcp": "Climate Vulnerability & Resilience Planning",

    // Principle 7: Policy Advocacy
    "p7_trade_associations": "Affiliation with Trade and Industry Chambers",
    "p7_anticompetitive_cases": "Anti-Competitive Behavior Complaints & Litigations",
    "p7_policy_advocacy": "Public Policy Advocacy & Research Submissions",

    // Principle 8: Inclusive Growth & Community
    "p8_msme_sourcing_current": "Procurement from MSMEs - Current FY (%)",
    "p8_msme_sourcing_prev": "Procurement from MSMEs - Previous FY (%)",
    "p8_local_sourcing_current": "Sourcing from Within District/Local Area - Current FY (%)",
    "p8_local_sourcing_prev": "Sourcing from Within District/Local Area - Previous FY (%)",
    "p8_community_grievance_mechanism": "Community Grievance Redressal Mechanism",
    "p8_negative_impact_mitigation": "Mitigation of Adverse Social Impacts on Local Communities",
    "p8_local_supplier_wellbeing": "Capacity Building & Well-being of Local Small Suppliers",

    // Principle 9: Consumer Responsibility
    "p9_labelled_products_turnover_pct": "Products Carrying Voluntary Sustainability Labels (%)",
    "p9_consumer_feedback_mechanism": "Consumer Feedback & Customer Satisfaction Framework",
    "p9_cyber_security_policy": "Cyber Security & Personal Data Protection Framework",
    "p9_corrective_actions": "Corrective Actions on Customer Complaints",
    "p9_info_channels": "Product Information Channels (Packaging, QR, Web)",
    "p9_consumer_education_steps": "Consumer Awareness on Safe & Sustainable Usage",
    "p9_service_disruption_mechanism": "Customer Grievance Channels for Service Disruption",
    "p9_additional_product_info": "Additional Voluntary Disclosures on Product Impact",
    "p9_data_privacy_survey_results": "Data Privacy Audits & Consumer Survey Results",

    // BRSR Core
    "assurance_statement_summary": "Independent Reasonable/Limited Assurance Statement Summary"
};

var tableTitles = {
    "products_sold": "Products & Services Sold (% of Turnover)",
    "business_activities": "Main Business Activities & Turnover Share",
    "subsidiaries": "Holding, Subsidiary and Joint Venture Companies",
    "csr_projects": "Corporate Social Responsibility (CSR) Projects",
    "stakeholder_grievances": "Stakeholder Grievances & Redressal Status",
    "penalties": "Fines & Penalties Imposed by Regulators",
    "material_issues": "Material ESG Issues & Opportunities",
    "turnover_rates": "Employee & Worker Turnover Rates",
    "principle_policy_matrix": "Principle-wise Policy Matrix (NGRBC)",
    "review_by_committee": "Committee Oversight & Reviews of NGRBC Principles",
    "external_assessment": "Independent Assessment of Working of Principles by External Agency",
    "gap_reasons": "Reasons for Not Formulating Policies",
    "p1_training": "Anti-Corruption & Ethics Training Coverage",
    "p1_fines": "Fines / Penalties on Ethics & Anti-Corruption",
    "p1_concentration": "Customer / Supplier Exposure & Concentration",
    "p2_reclaim": "Reclaimed Packaging & End-of-Life Products",
    "p2_lca": "Life Cycle Assessments (LCA) Conducted",
    "p2_recycled_input": "Recycled / Reused Input Materials",
    "p3_wellbeing": "Health, Accident & Maternity Well-being Coverage",
    "p3_safety_incidents": "Occupational Health Incidents (LTIFR)",
    "p3_retirement_benefits": "Statutory Retirement Benefits Coverage (PF, Gratuity)",
    "p3_parental_leave_retention": "Parental Leave Return & Retention Rates",
    "p3_grievance_mechanisms": "Working Conditions Grievance Mechanisms",
    "p3_union_membership": "Trade Union Representation",
    "p3_hs_training": "Health & Safety Training Programs",
    "p3_performance_reviews": "Performance & Career Development Appraisals",
    "p4_stakeholder_engagement": "Stakeholder Engagement & Consultation Channels",
    "p5_hr_training": "Human Rights Awareness & Training",
    "p5_minimum_wages": "Payment of Wages Equal to or Above Minimum Wages",
    "p5_remuneration": "Median Remuneration & Wage Parity",
    "p5_hr_complaints": "Human Rights Complaints & Grievances",
    "p5_hr_assessments": "Human Rights Due Diligence across Plants",
    "p6_energy": "Energy Consumption Breakdown (Renewable vs Non-Renewable)",
    "p6_air_emissions": "Air Emissions Parameters (NOx, SOx, PM, Others)",
    "p6_ghg_emissions": "Greenhouse Gas (GHG) Scope 1 & 2 Emissions",
    "p6_scope123_emissions": "GHG Scope 1, 2 and 3 Emissions Inventory",
    "p6_water_withdrawal": "Water Withdrawal by Source",
    "p6_water_discharge": "Water Discharge & Effluent Destination",
    "p6_waste": "Waste Generated, Recycled & Disposed",
    "p6_eia": "Environmental Impact Assessments (EIA) Undertaken",
    "p7_trade_associations": "Affiliations with Trade & Industry Associations",
    "p7_anticompetitive_cases": "Inquiries on Anti-Competitive Conduct",
    "p7_policy_advocacy": "Public Policy Advocacy Positions & Submissions",
    "p8_sia": "Social Impact Assessments (SIA) of Projects",
    "p8_rehabilitation": "Rehabilitation & Resettlement (R&R) Projects",
    "p8_csr_project_details": "CSR Project Locations & Expenditure",
    "p9_complaints": "Consumer Grievances & Complaints Received",
    "p9_product_recalls": "Product Recalls on Safety & Quality Grounds",
    "brsr_core_kpis": "BRSR Core Key Performance Indicators"
};

var tableSchemas = {
    "products_sold": [
        { key: "product_name", label: "Product / Service Name", flex: 3 },
        { key: "nic_code", label: "NIC Code", flex: 1 },
        { key: "turnover_pct", label: "% of Turnover", flex: 1 }
    ],
    "business_activities": [
        { key: "description", label: "Description of Main Business Activity", flex: 3 },
        { key: "turnover_pct", label: "% of Turnover", flex: 1 }
    ],
    "subsidiaries": [
        { key: "company_name", label: "Company / Subsidiary Name", flex: 3 },
        { key: "business_activity", label: "Type / Business Activity", flex: 2 },
        { key: "holding_pct", label: "% Shares Held", flex: 1 },
        { key: "br_participation", label: "Participates in BR (Yes/No)", flex: 1 }
    ],
    "csr_projects": [
        { key: "project_name", label: "CSR Project Name", flex: 3 },
        { key: "location", label: "State & District / Location", flex: 2 },
        { key: "amount_spent", label: "Amount Spent (₹ Cr/Lakhs)", flex: 1 }
    ],
    "stakeholder_grievances": [
        { key: "stakeholder_group", label: "Stakeholder Group", flex: 2 },
        { key: "received_current", label: "Complaints Received (Current FY)", flex: 1 },
        { key: "resolved_current", label: "Complaints Resolved (Current FY)", flex: 1 }
    ],
    "penalties": [
        { key: "authority", label: "Regulatory Authority", flex: 2 },
        { key: "amount", label: "Penalty Amount (₹)", flex: 1 },
        { key: "brief_case", label: "Brief Description of Case", flex: 3 }
    ],
    "material_issues": [
        { key: "material_issue", label: "Material ESG Issue", flex: 2 },
        { key: "rationale", label: "Rationale / Impact Area", flex: 3 },
        { key: "risk_or_opportunity", label: "Risk / Opportunity", flex: 1 }
    ],
    "turnover_rates": [
        { key: "category", label: "Workforce Category", flex: 2 },
        { key: "male_current", label: "Current Male (%)", flex: 1 },
        { key: "female_current", label: "Current Female (%)", flex: 1 },
        { key: "total_current", label: "Current Total (%)", flex: 1 },
        { key: "male_prev", label: "Prev Male (%)", flex: 1 },
        { key: "female_prev", label: "Prev Female (%)", flex: 1 },
        { key: "total_prev", label: "Prev Total (%)", flex: 1 }
    ],
    "principle_policy_matrix": [
        { key: "principle", label: "National Voluntary Principle", flex: 2 },
        { key: "policy_formulated", label: "Policy Formulated (Yes/No)", flex: 1 },
        { key: "board_approved", label: "Board Approved (Yes/No)", flex: 1 }
    ],
    "review_by_committee": [
        { key: "committee_name", label: "Board / Executive Committee", flex: 2 },
        { key: "subject", label: "Subject Reviewed", flex: 3 },
        { key: "frequency", label: "Frequency", flex: 1 }
    ],
    "external_assessment": [
        { key: "principle_name", label: "Principle", flex: 2 },
        { key: "assessed", label: "Independently Assessed (Yes/No)", flex: 1 },
        { key: "agency", label: "Name of Assessing Agency", flex: 3 }
    ],
    "gap_reasons": [
        { key: "principle", label: "Principle", flex: 1 },
        { key: "reason", label: "Reason for Not Formulating Policy", flex: 4 }
    ],
    "p1_training": [
        { key: "category", label: "Stakeholder Category", flex: 2 },
        { key: "anti_corruption_trained_pct", label: "Trained on Anti-Corruption (%)", flex: 2 }
    ],
    "p1_fines": [
        { key: "agency", label: "Regulatory Agency", flex: 2 },
        { key: "amount", label: "Fine Amount (₹)", flex: 1 },
        { key: "brief", label: "Brief Case Description", flex: 3 }
    ],
    "p1_concentration": [
        { key: "metric", label: "Counterparty Metric", flex: 2 },
        { key: "value", label: "Exposure / Share (%)", flex: 1 }
    ],
    "p2_reclaim": [
        { key: "material", label: "Material / Packaging Type", flex: 2 },
        { key: "reclaimed_pct", label: "Reclaimed / Collected (%)", flex: 1 }
    ],
    "p2_lca": [
        { key: "product_name", label: "Product / Service with LCA", flex: 3 },
        { key: "turnover_pct", label: "% of Turnover Contributed", flex: 1 }
    ],
    "p2_recycled_input": [
        { key: "material", label: "Recycled / Reused Material", flex: 2 },
        { key: "recycled_pct_current", label: "Recycled Content (%)", flex: 1 }
    ],
    "p3_wellbeing": [
        { key: "category", label: "Category (Employees / Workers)", flex: 2 },
        { key: "insurance_pct", label: "Health & Accident Cover (%)", flex: 1 },
        { key: "maternity_pct", label: "Maternity / Parental Cover (%)", flex: 1 }
    ],
    "p3_safety_incidents": [
        { key: "category", label: "Category (Employees / Workers)", flex: 2 },
        { key: "ltifr", label: "LTIFR (Lost Time Injury Rate)", flex: 1 },
        { key: "lost_days", label: "Lost Workdays", flex: 1 }
    ],
    "p3_retirement_benefits": [
        { key: "benefit", label: "Benefit (PF / Gratuity / ESI)", flex: 2 },
        { key: "emp_covered_pct", label: "Employees Covered (%)", flex: 1 },
        { key: "wrk_covered_pct", label: "Workers Covered (%)", flex: 1 }
    ],
    "p3_parental_leave_retention": [
        { key: "category", label: "Gender Category", flex: 2 },
        { key: "return_rate", label: "Return to Work Rate (%)", flex: 1 },
        { key: "retention_rate", label: "12-Month Retention Rate (%)", flex: 1 }
    ],
    "p3_grievance_mechanisms": [
        { key: "category", label: "Grievance Category", flex: 2 },
        { key: "mechanism_details", label: "Redressal Mechanism Details", flex: 3 }
    ],
    "p3_union_membership": [
        { key: "category", label: "Workforce Category", flex: 2 },
        { key: "union_members_pct", label: "Trade Union Members (%)", flex: 1 }
    ],
    "p3_hs_training": [
        { key: "category", label: "Workforce Category", flex: 2 },
        { key: "trained_no", label: "Number Trained", flex: 1 },
        { key: "trained_pct", label: "Trained Percentage (%)", flex: 1 }
    ],
    "p3_performance_reviews": [
        { key: "category", label: "Workforce Category", flex: 2 },
        { key: "male_pct", label: "Male Reviewed (%)", flex: 1 },
        { key: "female_pct", label: "Female Reviewed (%)", flex: 1 }
    ],
    "p4_stakeholder_engagement": [
        { key: "stakeholder_group", label: "Stakeholder Group", flex: 2 },
        { key: "consultation_channel", label: "Engagement Channel", flex: 2 },
        { key: "purpose", label: "Purpose & Topics Raised", flex: 3 }
    ],
    "p5_hr_training": [
        { key: "category", label: "Workforce Category", flex: 2 },
        { key: "employees_trained_pct", label: "Employees Trained (%)", flex: 1 },
        { key: "workers_trained_pct", label: "Workers Trained (%)", flex: 1 }
    ],
    "p5_minimum_wages": [
        { key: "category", label: "Workforce Category", flex: 2 },
        { key: "equal_or_more_pct", label: "Equal or > Min Wage (%)", flex: 1 },
        { key: "less_than_pct", label: "Less than Min Wage (%)", flex: 1 }
    ],
    "p5_remuneration": [
        { key: "category", label: "Level (Board / KMP / Other)", flex: 2 },
        { key: "male_median", label: "Male Median Pay (₹)", flex: 1 },
        { key: "female_median", label: "Female Median Pay (₹)", flex: 1 }
    ],
    "p5_hr_complaints": [
        { key: "category", label: "Complaint Category", flex: 2 },
        { key: "filed_current", label: "Filed in Current FY", flex: 1 },
        { key: "pending_current", label: "Pending Resolution", flex: 1 }
    ],
    "p5_hr_assessments": [
        { key: "aspect", label: "Human Rights Aspect Assessed", flex: 2 },
        { key: "plants_assessed_pct", label: "Plants Assessed (%)", flex: 1 }
    ],
    "p6_energy": [
        { key: "parameter", label: "Energy Source / Parameter", flex: 2 },
        { key: "current_value", label: "Current FY (GJ)", flex: 1 },
        { key: "previous_value", label: "Previous FY (GJ)", flex: 1 }
    ],
    "p6_air_emissions": [
        { key: "parameter", label: "Air Pollutant (NOx, SOx, PM)", flex: 2 },
        { key: "unit", label: "Unit", flex: 1 },
        { key: "current_value", label: "Current FY", flex: 1 },
        { key: "previous_value", label: "Previous FY", flex: 1 }
    ],
    "p6_ghg_emissions": [
        { key: "parameter", label: "GHG Scope / Source", flex: 2 },
        { key: "current_value", label: "Current FY (tCO₂e)", flex: 1 },
        { key: "previous_value", label: "Previous FY (tCO₂e)", flex: 1 }
    ],
    "p6_scope123_emissions": [
        { key: "scope", label: "GHG Scope (Scope 1 / 2 / 3)", flex: 2 },
        { key: "current_value", label: "Current FY (tCO₂e)", flex: 1 },
        { key: "previous_value", label: "Previous FY (tCO₂e)", flex: 1 }
    ],
    "p6_water_withdrawal": [
        { key: "parameter", label: "Water Source (Surface / Ground / Third Party)", flex: 2 },
        { key: "current_value", label: "Current FY (kL)", flex: 1 },
        { key: "previous_value", label: "Previous FY (kL)", flex: 1 }
    ],
    "p6_water_discharge": [
        { key: "destination", label: "Discharge Destination / Treatment", flex: 2 },
        { key: "current_value", label: "Current FY (kL)", flex: 1 },
        { key: "previous_value", label: "Previous FY (kL)", flex: 1 }
    ],
    "p6_waste": [
        { key: "category", label: "Waste Stream (Hazardous / Non-Hazardous)", flex: 2 },
        { key: "current_value", label: "Current FY (MT)", flex: 1 },
        { key: "previous_value", label: "Previous FY (MT)", flex: 1 }
    ],
    "p6_eia": [
        { key: "project_name", label: "Project Subject to EIA", flex: 2 },
        { key: "eia_notification_no", label: "Notification Number", flex: 2 },
        { key: "conducted_by", label: "Independent Agency", flex: 2 }
    ],
    "p7_trade_associations": [
        { key: "association_name", label: "Trade / Industry Association", flex: 3 },
        { key: "reach", label: "Scope (National / State)", flex: 1 }
    ],
    "p7_anticompetitive_cases": [
        { key: "agency", label: "Regulatory Agency (e.g. CCI)", flex: 2 },
        { key: "brief_case", label: "Brief Case Description", flex: 3 }
    ],
    "p7_policy_advocacy": [
        { key: "issue_advocated", label: "Policy Issue Advocated", flex: 3 },
        { key: "method", label: "Representation Method", flex: 2 }
    ],
    "p8_sia": [
        { key: "project_name", label: "Project Subject to SIA", flex: 2 },
        { key: "summary", label: "Social Impact Summary", flex: 3 },
        { key: "web_link", label: "Web Link", flex: 2 }
    ],
    "p8_rehabilitation": [
        { key: "project_name", label: "Project with R&R", flex: 2 },
        { key: "pafs_affected", label: "Project Affected Families (PAFs)", flex: 1 }
    ],
    "p8_csr_project_details": [
        { key: "project_name", label: "CSR Project", flex: 2 },
        { key: "state", label: "State", flex: 1 },
        { key: "district", label: "District", flex: 1 },
        { key: "amount_spent", label: "Amount Spent (₹ Lakhs)", flex: 1 }
    ],
    "p9_complaints": [
        { key: "category", label: "Complaint Category", flex: 2 },
        { key: "received_current", label: "Received in Current FY", flex: 1 },
        { key: "pending_current", label: "Pending Resolution", flex: 1 }
    ],
    "p9_product_recalls": [
        { key: "reason", label: "Reason for Recall", flex: 3 },
        { key: "number_recalled", label: "Units Recalled", flex: 1 }
    ],
    "brsr_core_kpis": [
        { key: "kpi_name", label: "BRSR Core Indicator", flex: 3 },
        { key: "current_value", label: "Current Assured Value", flex: 2 },
        { key: "assurance_provider", label: "Assurance Provider", flex: 2 }
    ]
};

function formatLabel(key) {
    if (!key) return "";
    if (fieldLabels[key]) return fieldLabels[key];
    
    // Fallback: smart formatter converting snake_case to Title Case with acronyms
    var acronyms = {
        "cin": "CIN", "fy": "FY", "csr": "CSR", "esg": "ESG", "ghg": "GHG", "kmp": "KMP",
        "msme": "MSME", "lca": "LCA", "eia": "EIA", "sia": "SIA", "zld": "ZLD", "pat": "PAT",
        "ohs": "OHS", "rd": "R&D", "ltifr": "LTIFR", "bcp": "BCP", "pct": "(%)", "no": "No.",
        "emp": "Employee", "wrk": "Worker", "perm": "Permanent", "da": "Differently-Abled",
        "prev": "Previous", "curr": "Current"
    };

    var parts = key.split("_");
    var res = [];
    for (var i = 0; i < parts.length; ++i) {
        var word = parts[i].toLowerCase();
        if (acronyms[word]) {
            res.push(acronyms[word]);
        } else if (word.length > 0) {
            res.push(word.charAt(0).toUpperCase() + word.slice(1));
        }
    }
    return res.join(" ");
}

function formatTableTitle(key) {
    if (tableTitles[key]) return tableTitles[key];
    return formatLabel(key) + " Table";
}

function getTableColumns(key, rows) {
    if (tableSchemas[key]) return tableSchemas[key];
    if (rows && rows.length > 0 && typeof rows[0] === 'object' && rows[0] !== null) {
        var cols = [];
        var obj = rows[0];
        for (var prop in obj) {
            cols.push({ key: prop, label: formatLabel(prop), flex: 1 });
        }
        if (cols.length > 0) return cols;
    }
    return [{ key: "value", label: "Value", flex: 1 }];
}

function cleanDisplayValue(val) {
    if (val === null || val === undefined || val === "null" || val === "N/A" || val === "-" || val === "<Not provided>") {
        return "";
    }
    if (typeof val === "object") {
        if (Array.isArray(val)) {
            return val.length + " item(s)";
        }
        var parts = [];
        for (var k in val) {
            var v = val[k];
            if (v !== null && v !== "" && v !== "null" && v !== undefined) {
                parts.push(formatLabel(k) + ": " + v);
            }
        }
        return parts.length > 0 ? parts.join(", ") : "";
    }
    return String(val);
}

function cleanFileUrl(rawUrl) {
    if (!rawUrl) return "";
    var decoded = decodeURIComponent(rawUrl.toString());
    if (decoded.indexOf("file:///") === 0) {
        var afterScheme = decoded.substring(8);
        if (/^[a-zA-Z]:/.test(afterScheme)) {
            return afterScheme;
        }
        return "/" + afterScheme;
    }
    if (decoded.indexOf("file://localhost/") === 0) {
        var afterHost = decoded.substring(17);
        if (/^[a-zA-Z]:/.test(afterHost)) {
            return afterHost;
        }
        return "/" + afterHost;
    }
    if (decoded.indexOf("file://") === 0) {
        return decoded.substring(7);
    }
    return decoded;
}

var JobStatus = {
    Pending: "PENDING",
    Processing: "PROCESSING",
    Completed: "COMPLETED",
    Failed: "FAILED",
    Cancelled: "CANCELLED"
};

var ExtractionStatus = {
    NotStarted: "NOT_STARTED",
    Extracting: "EXTRACTING",
    ReadyForReview: "READY_FOR_REVIEW",
    Completed: "COMPLETED",
    Accepted: "ACCEPTED",
    Failed: "FAILED",
    Cancelled: "CANCELLED"
};

var DocumentStatus = {
    Queued: "QUEUED",
    Processing: "PROCESSING",
    Ingested: "INGESTED",
    Failed: "FAILED"
};

function applyServerSchema(schemaObj) {
    if (!schemaObj) return;
    if (schemaObj.field_labels) {
        for (var k in schemaObj.field_labels) {
            fieldLabels[k] = schemaObj.field_labels[k];
        }
    }
    if (schemaObj.table_titles) {
        for (var t in schemaObj.table_titles) {
            tableTitles[t] = schemaObj.table_titles[t];
        }
    }
    if (schemaObj.table_schemas) {
        for (var s in schemaObj.table_schemas) {
            tableSchemas[s] = schemaObj.table_schemas[s];
        }
    }
}

#include "BrsrSchema.h"

namespace {

FieldGroup makeGroup(const QString& id, const QString& label, const QString& section,
                      const QString& ragQuery, const QStringList& dataKeys,
                      const QStringList& narrativeKeys, const QStringList& loopKeys)
{
    FieldGroup group;
    group.id = id;
    group.label = label;
    group.section = section;
    group.ragQuery = ragQuery;
    group.dataFieldKeys = dataKeys;
    group.narrativeFieldKeys = narrativeKeys;
    group.loopFieldKeys = loopKeys;
    return group;
}

// The full set of template.html placeholders, each defaulted to "N/A" (scalars) or an empty
// array (loop collections). This guarantees every template variable resolves to something even
// when a field-group's extraction found nothing for it, or hasn't run yet. Kept as one literal
// blob (rather than reconstructed from the FieldGroup catalog below) so the default coverage is
// exactly the template's real placeholder list, not whatever subset the catalog's authors
// remembered to group.
const char* kDefaultBrsrSchemaJson = R"json({"brsr_core_kpis":[],"business_activities":[],"csr_projects":[],"external_assessment":[],"gap_reasons":[],"material_issues":[],"p1_concentration":[],"p1_fines":[],"p1_training":[],"p2_lca":[],"p2_reclaim":[],"p2_recycled_input":[],"p3_grievance_mechanisms":[],"p3_hs_training":[],"p3_parental_leave_retention":[],"p3_performance_reviews":[],"p3_retirement_benefits":[],"p3_safety_incidents":[],"p3_union_membership":[],"p3_wellbeing":[],"p4_stakeholder_engagement":[],"p5_hr_assessments":[],"p5_hr_complaints":[],"p5_hr_training":[],"p5_minimum_wages":[],"p5_remuneration":[],"p6_air_emissions":[],"p6_eia":[],"p6_energy":[],"p6_ghg_emissions":[],"p6_scope123_emissions":[],"p6_waste":[],"p6_water_discharge":[],"p6_water_withdrawal":[],"p7_anticompetitive_cases":[],"p7_policy_advocacy":[],"p7_trade_associations":[],"p8_csr_project_details":[],"p8_rehabilitation":[],"p8_sia":[],"p9_complaints":[],"p9_product_recalls":[],"penalties":[],"principle_policy_matrix":[],"products_sold":[],"review_by_committee":[],"stakeholder_grievances":[],"subsidiaries":[],"turnover_rates":[],"customer_types":"N/A","debug_mode":"false","report_title":"Business Responsibility & Sustainability Report (BRSR)","scope_1_2_intensity_current":"N/A","p6_pat_scheme_details":"N/A","da_emp_perm_female_pct":"N/A","p8_msme_sourcing_prev":"N/A","p2_adverse_impact_mitigation":"N/A","p3_wc_filed_prev":"N/A","p9_cyber_security_policy":"N/A","p5_hr_in_contracts":"N/A","wrk_other_female_no":"N/A","international_total":"N/A","wrk_perm_male_pct":"N/A","wrk_perm_female_pct":"N/A","director_esg_statement":"N/A","p6_sustainability_projects":"N/A","csr_turnover_amount":"N/A","p2_capex_improvements":"N/A","da_wrk_other_male_pct":"N/A","p6_value_chain_env_assessment_pct":"N/A","p3_transition_assistance_programs":"N/A","p9_service_disruption_mechanism":"N/A","emp_perm_male_no":"N/A","da_emp_perm_female_no":"N/A","esg_assessment_scope":"N/A","da_wrk_perm_female_pct":"N/A","p6_circular_economy_initiatives":"N/A","da_emp_other_male_pct":"N/A","da_emp_perm_total":"N/A","p6_climate_bcp":"N/A","p8_local_sourcing_current":"N/A","p3_corrective_actions":"N/A","da_wrk_perm_male_no":"N/A","p3_wc_pending_current":"N/A","p3_accessibility_details":"N/A","markets_national_states":"N/A","emp_perm_male_pct":"N/A","p8_local_supplier_wellbeing":"N/A","da_emp_other_female_no":"N/A","wrk_total_female_pct":"N/A","p4_consultation_process":"N/A","p4_stakeholder_identification_process":"N/A","p8_msme_sourcing_current":"N/A","p3_hs_pending_prev":"N/A","entity_name":"N/A","contact_email":"N/A","paid_up_capital":"N/A","da_wrk_female_no":"N/A","p9_consumer_feedback_mechanism":"N/A","p6_zld_status":"N/A","p6_value_chain_adverse_impact":"N/A","p5_anti_retaliation_mechanism":"N/A","p1_accounts_payable_days":"N/A","da_wrk_other_total":"N/A","national_total":"N/A","wrk_total_male_pct":"N/A","p9_info_channels":"N/A","csr_unspent_amount":"N/A","p6_business_continuity_plan":"N/A","da_wrk_perm_male_pct":"N/A","assurance_statement_summary":"N/A","p5_focal_point_details":"N/A","p2_rd_improvements":"N/A","da_emp_perm_male_pct":"N/A","sustainability_committee_details":"N/A","emp_total_female_no":"N/A","da_emp_female_pct":"N/A","p3_hs_pending_current":"N/A","p6_certifications_audits":"N/A","da_emp_male_no":"N/A","contact_telephone":"N/A","p2_rd_pct_current":"N/A","contact_person_details":"N/A","da_wrk_other_female_pct":"N/A","da_wrk_male_no":"N/A","p3_safe_workplace_measures":"N/A","da_wrk_perm_female_no":"N/A","p2_epr_status":"N/A","da_emp_perm_male_no":"N/A","p3_equal_opportunity_policy":"N/A","p5_value_chain_hr_assessment":"N/A","da_emp_other_total":"N/A","wrk_perm_total":"N/A","p3_wc_filed_current":"N/A","prior_to_previous_fy":"N/A","p3_hs_filed_prev":"N/A","p3_wc_pending_prev":"N/A","p1_conflict_of_interest_process":"N/A","wrk_total_male_no":"N/A","markets_international_countries":"N/A","p1_value_chain_awareness":"N/A","emp_total_male_no":"N/A","year_of_incorporation":"N/A","previous_renewable_energy_pct":"N/A","entity_cin":"N/A","p4_vulnerable_groups_consultation":"N/A","wrk_perm_female_no":"N/A","esg_assessment_done":"N/A","wrk_perm_male_no":"N/A","p4_consultation_outcomes":"N/A","p3_death_compensation_details":"N/A","p9_additional_product_info":"N/A","emp_other_male_pct":"N/A","p6_water_stress_area_discharge":"N/A","p6_ghg_reduction_projects":"N/A","current_renewable_energy_pct":"N/A","csr_spent_amount":"N/A","website_url":"N/A","da_emp_male_pct":"N/A","p8_local_sourcing_prev":"N/A","csr_applicable":"N/A","p3_value_chain_grievance_mechanism":"N/A","kmp_total":"N/A","p6_water_intensity_turnover_current":"N/A","da_wrk_total":"N/A","emp_total_male_pct":"N/A","da_emp_other_male_no":"N/A","emp_other_female_pct":"N/A","emp_perm_female_pct":"N/A","reporting_boundary":"N/A","p3_value_chain_assessment":"N/A","p5_grievance_mechanism":"N/A","p8_community_grievance_mechanism":"N/A","csr_net_worth":"N/A","da_wrk_perm_total":"N/A","assurance_provider_name":"N/A","emp_perm_total":"N/A","p8_negative_impact_mitigation":"N/A","governance_oversight_statement":"N/A","da_emp_total":"N/A","wrk_other_total":"N/A","emp_total_total":"N/A","p2_capex_pct_current":"N/A","p3_ohs_system_implemented":"N/A","da_wrk_other_female_no":"N/A","emp_other_total":"N/A","emp_other_male_no":"N/A","national_plants":"N/A","esg_assessment_agency":"N/A","board_female_pct":"N/A","kmp_female_no":"N/A","wrk_other_male_pct":"N/A","da_emp_female_no":"N/A","reporting_financial_year":"N/A","p9_corrective_actions":"N/A","board_female_no":"N/A","p9_data_privacy_survey_results":"N/A","international_plants":"N/A","previous_fy":"N/A","emp_other_female_no":"N/A","emp_perm_female_no":"N/A","p5_process_modification":"N/A","p3_hazard_reporting_process":"N/A","highest_authority_details":"N/A","assurance_type":"N/A","p3_value_chain_parental_leave":"N/A","p6_value_chain_corrective_actions":"N/A","p3_hazard_identification_process":"N/A","p6_environmental_compliance":"N/A","corporate_address":"N/A","p9_consumer_education_steps":"N/A","kmp_female_pct":"N/A","p2_sustainable_sourcing_process":"N/A","csr_obligation_amount":"N/A","da_wrk_male_pct":"N/A","wrk_other_female_pct":"N/A","wrk_total_female_no":"N/A","p2_rd_pct_prev":"N/A","registered_address":"N/A","p3_hs_filed_current":"N/A","p3_disability_inclusion_measures":"N/A","stock_exchanges":"N/A","p3_healthcare_access":"N/A","scope_1_2_intensity_prev":"N/A","p9_labelled_products_turnover_pct":"N/A","international_offices":"N/A","da_wrk_female_pct":"N/A","national_offices":"N/A","p2_sustainable_sourcing_pct":"N/A","da_emp_other_female_pct":"N/A","p5_corrective_actions":"N/A","export_contribution_pct":"N/A","board_total":"N/A","p6_low_carbon_transition_plan":"N/A","emp_total_female_pct":"N/A","wrk_total_total":"N/A","sustainability_decision_committee_details":"N/A","p6_water_intensity_turnover_prev":"N/A","wrk_other_male_no":"N/A","current_fy":"N/A","da_wrk_other_male_no":"N/A","p2_capex_pct_prev":"N/A"})json";

} // namespace

BrsrSchemaCatalog::BrsrSchemaCatalog()
{
    _groups = {
        makeGroup("section_a_corporate", "Section A: Corporate Identity & Capital", "A",
            "corporate identity number CIN entity name year of incorporation registered office corporate address email telephone website stock exchanges listing paid up capital reporting financial year",
            {"entity_cin", "entity_name", "year_of_incorporation", "registered_address", "corporate_address",
             "contact_email", "contact_telephone", "website_url", "reporting_financial_year", "stock_exchanges",
             "paid_up_capital", "contact_person_details", "reporting_boundary", "assurance_provider_name",
             "assurance_type", "current_fy", "previous_fy", "prior_to_previous_fy"},
            {"customer_types"},
            {}),

        makeGroup("section_a_operations", "Section A: Operations, Markets & CSR", "A",
            "number of plants offices locations national international markets states countries exports contribution turnover business activities products sold subsidiaries CSR net worth obligation spent penalties",
            {"national_plants", "national_offices", "national_total", "international_plants", "international_offices",
             "international_total", "markets_national_states", "markets_international_countries", "export_contribution_pct",
             "csr_applicable", "csr_turnover_amount", "csr_net_worth", "csr_obligation_amount", "csr_spent_amount", "csr_unspent_amount",
             "esg_assessment_done", "esg_assessment_agency", "esg_assessment_scope"},
            {},
            {"business_activities", "products_sold", "subsidiaries", "csr_projects", "stakeholder_grievances", "penalties", "material_issues"}),

        makeGroup("section_a_employees", "Section A: Employees & Governance Diversity", "A",
            "employees permanent other male female total percentage turnover rates board of directors key managerial personnel women representation",
            {"emp_perm_total", "emp_perm_male_no", "emp_perm_male_pct", "emp_perm_female_no", "emp_perm_female_pct",
             "emp_other_total", "emp_other_male_no", "emp_other_male_pct", "emp_other_female_no", "emp_other_female_pct",
             "emp_total_total", "emp_total_male_no", "emp_total_male_pct", "emp_total_female_no", "emp_total_female_pct",
             "board_total", "board_female_no", "board_female_pct", "kmp_total", "kmp_female_no", "kmp_female_pct"},
            {},
            {"turnover_rates"}),

        makeGroup("section_a_workers", "Section A: Workers & Differently-Abled", "A",
            "workers permanent other male female total differently abled employees and workers representation percentage",
            {"wrk_perm_total", "wrk_perm_male_no", "wrk_perm_male_pct", "wrk_perm_female_no", "wrk_perm_female_pct",
             "wrk_other_total", "wrk_other_male_no", "wrk_other_male_pct", "wrk_other_female_no", "wrk_other_female_pct",
             "wrk_total_total", "wrk_total_male_no", "wrk_total_male_pct", "wrk_total_female_no", "wrk_total_female_pct",
             "da_emp_perm_total", "da_emp_perm_male_no", "da_emp_perm_male_pct", "da_emp_perm_female_no", "da_emp_perm_female_pct",
             "da_emp_other_total", "da_emp_other_male_no", "da_emp_other_male_pct", "da_emp_other_female_no", "da_emp_other_female_pct",
             "da_emp_total", "da_emp_male_no", "da_emp_male_pct", "da_emp_female_no", "da_emp_female_pct",
             "da_wrk_perm_total", "da_wrk_perm_male_no", "da_wrk_perm_male_pct", "da_wrk_perm_female_no", "da_wrk_perm_female_pct",
             "da_wrk_other_total", "da_wrk_other_male_no", "da_wrk_other_male_pct", "da_wrk_other_female_no", "da_wrk_other_female_pct",
             "da_wrk_total", "da_wrk_male_no", "da_wrk_male_pct", "da_wrk_female_no", "da_wrk_female_pct"},
            {},
            {}),

        makeGroup("section_b_governance", "Section B: Governance & Oversight", "B",
            "director statement sustainability ESG highest authority responsible sustainability committee governance oversight review committee external assessment",
            {},
            {"director_esg_statement", "highest_authority_details", "sustainability_committee_details",
             "governance_oversight_statement", "sustainability_decision_committee_details"},
            {"review_by_committee", "external_assessment", "gap_reasons"}),

        makeGroup("section_b_policies", "Section B: Principle Policy Matrix", "B",
            "national voluntary guidelines business responsibility policy covers principle board approved statutory commitments value chain web link",
            {},
            {},
            {"principle_policy_matrix"}),

        makeGroup("principle_1", "Principle 1: Ethics, Transparency & Accountability", "P1",
            "anti-corruption training fines penalties conflict of interest accounts payable days value chain awareness",
            {"p1_accounts_payable_days"},
            {"p1_value_chain_awareness", "p1_conflict_of_interest_process"},
            {"p1_training", "p1_fines", "p1_concentration"}),

        makeGroup("principle_2", "Principle 2: Sustainable Goods & Services", "P2",
            "research and development capital expenditure sustainable technologies sourcing extended producer responsibility reclaimed materials life cycle assessment recycled input",
            {"p2_rd_pct_current", "p2_rd_pct_prev", "p2_capex_pct_current", "p2_capex_pct_prev",
             "p2_sustainable_sourcing_process", "p2_sustainable_sourcing_pct"},
            {"p2_rd_improvements", "p2_capex_improvements", "p2_epr_status", "p2_adverse_impact_mitigation"},
            {"p2_reclaim", "p2_lca", "p2_recycled_input"}),

        makeGroup("principle_3", "Principle 3: Employee Wellbeing & Safety", "P3",
            "occupational health safety system hazard identification reporting workplace accessibility equal opportunity compensation retirement benefits parental leave retention union training corrective actions",
            {"p3_ohs_system_implemented", "p3_wc_filed_current", "p3_wc_pending_current", "p3_wc_filed_prev", "p3_wc_pending_prev",
             "p3_hs_filed_current", "p3_hs_pending_current", "p3_hs_filed_prev", "p3_hs_pending_prev"},
            {"p3_accessibility_details", "p3_equal_opportunity_policy", "p3_hazard_identification_process",
             "p3_hazard_reporting_process", "p3_healthcare_access", "p3_safe_workplace_measures",
             "p3_value_chain_assessment", "p3_corrective_actions", "p3_death_compensation_details",
             "p3_disability_inclusion_measures", "p3_value_chain_parental_leave",
             "p3_value_chain_grievance_mechanism", "p3_transition_assistance_programs"},
            {"p3_wellbeing", "p3_safety_incidents", "p3_retirement_benefits", "p3_parental_leave_retention",
             "p3_grievance_mechanisms", "p3_union_membership", "p3_hs_training", "p3_performance_reviews"}),

        makeGroup("principle_4", "Principle 4: Stakeholder Engagement", "P4",
            "stakeholder identification process consultation vulnerable marginalized groups engagement outcomes",
            {},
            {"p4_stakeholder_identification_process", "p4_consultation_process",
             "p4_vulnerable_groups_consultation", "p4_consultation_outcomes"},
            {"p4_stakeholder_engagement"}),

        makeGroup("principle_5", "Principle 5: Human Rights", "P5",
            "human rights focal point grievance mechanism anti-retaliation minimum wages remuneration training complaints assessments",
            {},
            {"p5_focal_point_details", "p5_grievance_mechanism", "p5_anti_retaliation_mechanism",
             "p5_hr_in_contracts", "p5_corrective_actions", "p5_process_modification", "p5_value_chain_hr_assessment"},
            {"p5_hr_training", "p5_minimum_wages", "p5_remuneration", "p5_hr_complaints", "p5_hr_assessments"}),

        makeGroup("principle_6_emissions", "Principle 6: Energy & GHG Emissions", "P6",
            "energy consumption renewable non-renewable sources greenhouse gas emissions scope 1 scope 2 scope 3 air emissions reduction projects low carbon transition",
            {"current_renewable_energy_pct", "previous_renewable_energy_pct",
             "scope_1_2_intensity_current", "scope_1_2_intensity_prev"},
            {"p6_ghg_reduction_projects", "p6_low_carbon_transition_plan"},
            {"p6_energy", "p6_air_emissions", "p6_ghg_emissions", "p6_scope123_emissions"}),

        makeGroup("principle_6_water_waste", "Principle 6: Water, Waste & Environmental Compliance", "P6",
            "water withdrawal consumption discharge zero liquid discharge ZLD effluent waste generated circular economy environmental compliance EIA certifications audits",
            {"p6_water_intensity_turnover_current", "p6_water_intensity_turnover_prev",
             "p6_value_chain_env_assessment_pct"},
            {"p6_pat_scheme_details", "p6_zld_status", "p6_water_stress_area_discharge",
             "p6_environmental_compliance", "p6_circular_economy_initiatives", "p6_sustainability_projects",
             "p6_business_continuity_plan", "p6_value_chain_adverse_impact", "p6_value_chain_corrective_actions",
             "p6_certifications_audits", "p6_climate_bcp"},
            {"p6_water_withdrawal", "p6_water_discharge", "p6_waste", "p6_eia"}),

        makeGroup("principle_7", "Principle 7: Policy Advocacy", "P7",
            "trade and industry associations anti-competitive conduct public policy advocacy positions",
            {}, {},
            {"p7_trade_associations", "p7_anticompetitive_cases", "p7_policy_advocacy"}),

        makeGroup("principle_8", "Principle 8: Inclusive Growth & Community", "P8",
            "social impact assessment rehabilitation resettlement CSR projects MSME local sourcing community grievance",
            {"p8_msme_sourcing_current", "p8_msme_sourcing_prev", "p8_local_sourcing_current", "p8_local_sourcing_prev"},
            {"p8_community_grievance_mechanism", "p8_negative_impact_mitigation", "p8_local_supplier_wellbeing"},
            {"p8_sia", "p8_rehabilitation", "p8_csr_project_details"}),

        makeGroup("principle_9", "Principle 9: Consumer Responsibility", "P9",
            "consumer complaints product recalls feedback mechanism cyber security data privacy product labelling information",
            {"p9_labelled_products_turnover_pct"},
            {"p9_consumer_feedback_mechanism", "p9_cyber_security_policy", "p9_corrective_actions", "p9_info_channels",
             "p9_consumer_education_steps", "p9_service_disruption_mechanism", "p9_additional_product_info",
             "p9_data_privacy_survey_results"},
            {"p9_complaints", "p9_product_recalls"}),

        makeGroup("brsr_core", "BRSR Core Assurance", "CORE",
            "BRSR core key performance indicators assurance statement independent assurance provider",
            {},
            {"assurance_statement_summary"},
            {"brsr_core_kpis"}),
    };
}

const FieldGroup* BrsrSchemaCatalog::findGroup(const QString& groupId) const
{
    for (const FieldGroup& group : _groups) {
        if (group.id == groupId) {
            return &group;
        }
    }
    return nullptr;
}

QStringList BrsrSchemaCatalog::sectionOrder()
{
    return {"A", "B", "P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8", "P9", "CORE"};
}

nlohmann::json BrsrSchemaCatalog::defaultSchema() const
{
    static const nlohmann::json kSchema = nlohmann::json::parse(kDefaultBrsrSchemaJson);
    return kSchema;
}
